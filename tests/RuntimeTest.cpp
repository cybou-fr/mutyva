#include <QtTest>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include "DeclarativeRuntime.h"
#include "AppDocument.h"
#include "WeatherService.h"

namespace {
QJsonObject read(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
QJsonObject baseline() { return read(":/examples/city-pulse.mutyva.json"); }
QJsonObject proposal(const QJsonArray &operations) {
    return {{"schemaVersion", 1}, {"appId", "official.city-pulse"}, {"operations", operations}};
}
QString json(const QJsonObject &object) { return QString::fromUtf8(QJsonDocument(object).toJson()); }
QByteArray bytes(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}
bool hasText(QQuickItem *item, const QString &text) {
    if (item->property("text").toString() == text) return true;
    for (auto *child : item->childItems()) if (hasText(child, text)) return true;
    return false;
}
}

class RuntimeTest : public QObject {
    Q_OBJECT
private slots:
    void bundledSchemasAndCatalog() {
        QString error;
        QVERIFY2(DeclarativeRuntime::validateDocument(baseline(), error), qPrintable(error));
        QCOMPARE(DeclarativeRuntime::catalog().size(), 5);
        for (const auto &entry : DeclarativeRuntime::catalog()) {
            auto doc = baseline();
            auto widgets = doc.value("widgets").toArray();
            widgets.append(QJsonObject{{"id", "testWidget"}, {"kind", entry.toMap().value("kind").toString()}});
            doc.insert("widgets", widgets);
            QVERIFY2(DeclarativeRuntime::validateDocument(doc, error), qPrintable(error));
        }
    }

    void invalidDocuments_data() {
        QTest::addColumn<QJsonObject>("document");
        auto add = [](const char *name, const QJsonObject &doc) { QTest::newRow(name) << doc; };
        auto doc = baseline(); doc.remove("name"); add("required", doc);
        doc = baseline(); doc.insert("schemaVersion", 1.5); add("version", doc);
        doc = baseline(); doc.insert("widgets", "wrong"); add("array-type", doc);
        doc = baseline(); doc.insert("executable", "shell"); add("extra-property", doc);
        auto widget = baseline().value("widgets").toArray()[0].toObject();
        int badIndex = 0;
        for (const auto &bad : QJsonArray{QJsonObject{{"kind", "qml-code"}},
                                         QJsonObject{{"title", 42}}, QJsonObject{{"title", QString(121, 'x')}},
                                         QJsonObject{{"id", "../bad"}}, QJsonObject{{"onClick", "exec()"}}}) {
            auto w = widget;
            const auto patch = bad.toObject();
            for (auto it = patch.begin(); it != patch.end(); ++it) w.insert(it.key(), it.value());
            doc = baseline(); doc.insert("widgets", QJsonArray{w});
            add(qPrintable(QString("widget-%1-%2").arg(patch.begin().key()).arg(badIndex++)), doc);
        }
        doc = baseline(); doc.insert("widgets", QJsonArray{widget, widget}); add("duplicate-id", doc);
        doc = baseline(); doc.insert("widgets", QJsonArray{false}); add("invalid-widget", doc);
        QJsonArray many;
        for (int i = 0; i < 101; ++i) many.append(QJsonObject{{"id", "widget" + QString::number(i)}, {"kind", "weather-card"}});
        doc = baseline(); doc.insert("widgets", many); add("widget-limit", doc);
        doc = baseline(); doc.insert("dataSources", QJsonArray{}); add("missing-binding-source", doc);
        auto sources = baseline().value("dataSources").toArray(); sources.append(sources[0]);
        doc = baseline(); doc.insert("dataSources", sources); add("duplicate-source", doc);
        doc = baseline(); doc.insert("permissions", QJsonArray{"filesystem:*"}); add("unapproved-permission", doc);
        doc = baseline(); doc.insert("permissions", QJsonArray{}); add("missing-permission", doc);
        doc = baseline(); doc.insert("permissions", QJsonArray{"network:*.open-meteo.com", "network:*.open-meteo.com"});
        add("duplicate-permission", doc);
    }

    void invalidDocuments() {
        QFETCH(QJsonObject, document);
        QString error;
        QVERIFY(!DeclarativeRuntime::validateDocument(document, error));
        QVERIFY(!error.isEmpty());
    }

    void sequentialOperations() {
        const auto doc = baseline();
        const auto original = doc;
        const QJsonArray operations{
            QJsonObject{{"op", "addWidget"}, {"after", "forecast"}, {"widget", QJsonObject{
                {"id", "humidity"}, {"kind", "humidity-card"}, {"title", "Влажность"}}}},
            QJsonObject{{"op", "updateProperty"}, {"id", "humidity"}, {"path", "title"}, {"value", "Мой заголовок"}},
            QJsonObject{{"op", "moveWidget"}, {"id", "humidity"}, {"after", ""}},
            QJsonObject{{"op", "removeWidget"}, {"id", "air"}}
        };
        QJsonObject result;
        QString error;
        QVERIFY2(DeclarativeRuntime::mutate(doc, proposal(operations), result, error), qPrintable(error));
        const auto widgets = result.value("widgets").toArray();
        QCOMPARE(widgets.size(), 4);
        QCOMPARE(widgets[0].toObject().value("id").toString(), "humidity");
        QCOMPARE(widgets[0].toObject().value("title").toString(), QString::fromUtf8("Мой заголовок"));
        QCOMPARE(widgets.last().toObject().value("id").toString(), "forecast");
        QCOMPARE(doc, original);
    }

    void invalidMutations_data() {
        QTest::addColumn<QJsonObject>("mutation");
        const auto fixture = read(":/examples/mutations/add-humidity-card.json");
        auto m = fixture; m.insert("appId", "other.app"); QTest::newRow("wrong-app") << m;
        m = fixture; m.insert("operations", QJsonArray{}); QTest::newRow("empty-ops") << m;
        m = fixture; m.insert("permissions", QJsonArray{"filesystem:*"}); QTest::newRow("extra-fields") << m;
        const QJsonArray badOps{
            QJsonObject{{"op", "execute"}, {"id", "forecast"}},
            QJsonObject{{"op", "removeWidget"}, {"id", "missing"}},
            QJsonObject{{"op", "updateProperty"}, {"id", "forecast"}, {"path", "kind"}, {"value", "city-search"}},
            QJsonObject{{"op", "updateProperty"}, {"id", "forecast"}, {"path", "title"}, {"value", 42}},
            QJsonObject{{"op", "moveWidget"}, {"id", "forecast"}, {"after", "forecast"}},
            QJsonObject{{"op", "moveWidget"}, {"id", "forecast"}, {"after", "missing"}},
            QJsonObject{{"op", "addWidget"}, {"widget", QJsonObject{{"id", "forecast"}, {"kind", "weather-card"}}}},
            QJsonObject{{"op", "addWidget"}, {"after", "missing"}, {"widget", QJsonObject{{"id", "newWidget"}, {"kind", "weather-card"}}}}
        };
        int i = 0;
        for (const auto &op : badOps) {
            // First operation succeeds; second must reject the entire batch.
            auto operations = fixture.value("operations").toArray();
            operations.append(op);
            QTest::newRow(qPrintable(QString("invalid-op-%1").arg(i++))) << proposal(operations);
        }
        QJsonArray tooMany;
        for (i = 0; i < 21; ++i) tooMany.append(QJsonObject{{"op", "removeWidget"}, {"id", "forecast"}});
        QTest::newRow("op-limit") << proposal(tooMany);
    }

    void invalidMutations() {
        QFETCH(QJsonObject, mutation);
        const auto doc = baseline();
        QJsonObject result{{"untouched", true}};
        const auto original = result;
        QString error;
        QVERIFY(!DeclarativeRuntime::mutate(doc, mutation, result, error));
        QCOMPARE(result, original);
        QCOMPARE(doc, baseline());
        QVERIFY(!error.isEmpty());
    }

    void previewApplyCancelRestart() {
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        AppDocument document(nullptr, folder.path());
        QCOMPARE(document.generation(), 1);
        const auto initial = document.documentJson();
        const auto head = bytes(folder.filePath("head.json"));
        const auto snapshots = QDir(folder.path()).entryList(QDir::Files);
        QVERIFY2(document.addHumidityCard(), qPrintable(document.lastError()));
        QVERIFY(document.hasPreview());
        QCOMPARE(document.previewWidgets().size(), 5);
        QCOMPARE(document.documentJson(), initial);
        QCOMPARE(bytes(folder.filePath("head.json")), head);
        QCOMPARE(QDir(folder.path()).entryList(QDir::Files), snapshots);
        document.cancelPreview();
        QVERIFY(!document.hasPreview());
        QVERIFY(!document.applyPreview());
        QCOMPARE(document.generation(), 1);
        QVERIFY(!document.previewMutation("{broken"));
        QVERIFY(!document.previewDocument("[]"));
        QVERIFY(!document.previewDocument(QString(1024 * 1024 + 1, 'x')));
        QCOMPARE(QDir(folder.path()).entryList(QDir::Files), snapshots);
        QVERIFY(document.addHumidityCard());
        QVERIFY2(document.applyPreview(), qPrintable(document.lastError()));
        QCOMPARE(document.generation(), 2);
        QCOMPARE(document.widgets().size(), 5);
        QVERIFY(!document.hasPreview());
        AppDocument reopened(nullptr, folder.path());
        QCOMPARE(reopened.generation(), 2);
        QCOMPARE(reopened.documentJson(), document.documentJson());
        QVERIFY(reopened.renameWidget("humidity", "Новая влажность"));
        QVERIFY(reopened.applyPreview());
        QVERIFY(reopened.moveWidget("humidity", ""));
        QVERIFY(reopened.applyPreview());
        QCOMPARE(reopened.widgets()[0].toMap().value("id").toString(), "humidity");
        QVERIFY(reopened.removeWidget("humidity"));
        QVERIFY(reopened.applyPreview());
        QCOMPARE(reopened.widgets().size(), 4);
        QCOMPARE(bytes(folder.filePath("generation-00001.json")), initial.toUtf8());
    }

    void documentEditingAndPermissionGate() {
        QTemporaryDir folder;
        AppDocument document(nullptr, folder.path());
        auto edited = baseline();
        auto widgets = edited.value("widgets").toArray();
        auto widget = widgets[0].toObject(); widget.insert("title", "Мой поиск"); widgets[0] = widget;
        edited.insert("widgets", widgets);
        QVERIFY(document.previewDocument(json(edited)));
        QVERIFY(document.applyPreview());
        edited.insert("id", "different.app");
        QVERIFY(!document.previewDocument(json(edited)));
        QVERIFY(!document.hasPreview());
        edited = baseline(); edited.insert("widgets", QJsonArray{});
        edited.insert("dataSources", QJsonArray{}); edited.insert("permissions", QJsonArray{});
        QVERIFY(!document.previewDocument(json(edited)));
        QCOMPARE(document.generation(), 2);
    }

    void staleWriterAndLock() {
        QTemporaryDir folder;
        AppDocument first(nullptr, folder.path()), second(nullptr, folder.path());
        QVERIFY(second.renameWidget("forecast", "Другой прогноз"));
        QVERIFY(first.addHumidityCard());
        QLockFile lock(folder.filePath("write.lock"));
        QVERIFY(lock.tryLock());
        QVERIFY(!first.applyPreview());
        QCOMPARE(first.generation(), 1);
        lock.unlock();
        QVERIFY(first.applyPreview());
        const auto head = bytes(folder.filePath("head.json"));
        QVERIFY(!second.applyPreview());
        QCOMPARE(second.generation(), 1);
        QCOMPARE(bytes(folder.filePath("head.json")), head);
    }

    void orphanAndCorruptHistory() {
        QTemporaryDir folder;
        AppDocument document(nullptr, folder.path());
        QFile orphan(folder.filePath("generation-00002.json"));
        QVERIFY(orphan.open(QIODevice::WriteOnly)); orphan.write("orphan"); orphan.close();
        QVERIFY(document.addHumidityCard());
        QVERIFY(document.applyPreview());
        QCOMPARE(document.generation(), 3);
        QCOMPARE(bytes(orphan.fileName()), QByteArray("orphan"));
        QFile snapshot(folder.filePath("generation-00003.json"));
        QVERIFY(snapshot.open(QIODevice::WriteOnly | QIODevice::Truncate));
        snapshot.write("{\"schemaVersion\":1,\"widgets\":[]}"); snapshot.close();
        const auto head = bytes(folder.filePath("head.json"));
        AppDocument reopened(nullptr, folder.path());
        QVERIFY(!reopened.lastError().isEmpty());
        QVERIFY(reopened.addHumidityCard());
        QVERIFY(!reopened.applyPreview());
        QCOMPARE(bytes(folder.filePath("head.json")), head);
        QCOMPARE(bytes(orphan.fileName()), QByteArray("orphan"));
    }

    void qmlPreviewAndTitles() {
        QTemporaryDir folder;
        AppDocument document(nullptr, folder.path());
        WeatherService weather;
        QQmlApplicationEngine engine;
        QStringList warnings;
        connect(&engine, &QQmlEngine::warnings, this, [&warnings](const QList<QQmlError> &errors) {
            for (const auto &error : errors) warnings << error.toString();
        });
        engine.rootContext()->setContextProperty("appDocument", &document);
        engine.rootContext()->setContextProperty("weatherService", &weather);
        engine.load(QUrl::fromLocalFile(QStringLiteral(MUTYVA_SOURCE_DIR "/starter/qml/Main.qml")));
        QVERIFY2(!engine.rootObjects().isEmpty(), qPrintable(warnings.join('\n')));
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects()[0]);
        QVERIFY(window);
        auto *repeater = window->findChild<QObject *>("widgetRepeater");
        QVERIFY(repeater);
        QTRY_COMPARE(repeater->property("count").toInt(), 4);
        QVERIFY(document.addHumidityCard());
        QTRY_COMPARE(repeater->property("count").toInt(), 5);
        QTRY_VERIFY(hasText(window->contentItem(), "Влажность воздуха"));
        document.cancelPreview();
        QTRY_COMPARE(repeater->property("count").toInt(), 4);
        QVERIFY(document.addHumidityCard());
        QVERIFY(document.applyPreview());
        for (const auto &entry : document.widgets()) {
            const auto id = entry.toMap().value("id").toString();
            const auto title = "<b>Текст</b> " + id;
            QVERIFY(document.renameWidget(id, title));
            QTRY_VERIFY(hasText(window->contentItem(), title));
            QVERIFY(document.applyPreview());
        }
        window->setWidth(360);
        window->setProperty("editorOpen", true);
        QTest::qWait(100);
        // Exercise the actual QML button handlers, not only the C++ API.
        auto *titleInput = window->findChild<QObject *>("titleInput");
        auto *renameButton = window->findChild<QObject *>("renameWidget");
        auto *applyButton = window->findChild<QObject *>("applyMutation");
        QVERIFY(titleInput && renameButton && applyButton);
        titleInput->setProperty("text", "Поиск моего города");
        QVERIFY(QMetaObject::invokeMethod(renameButton, "clicked"));
        QVERIFY(document.hasPreview());
        QTRY_VERIFY(hasText(window->contentItem(), "Поиск моего города"));
        QVERIFY(QMetaObject::invokeMethod(applyButton, "clicked"));
        QVERIFY(!document.hasPreview());
        QCOMPARE(document.widgets()[0].toMap().value("title").toString(), QString::fromUtf8("Поиск моего города"));
        auto *picker = window->findChild<QObject *>("widgetPicker");
        auto *moveButton = window->findChild<QObject *>("moveWidget");
        auto *removeButton = window->findChild<QObject *>("removeWidget");
        QVERIFY(picker && moveButton && removeButton);
        picker->setProperty("currentIndex", 1);
        QVERIFY(QMetaObject::invokeMethod(moveButton, "clicked"));
        QVERIFY(document.hasPreview());
        QCOMPARE(document.previewWidgets()[0].toMap().value("id").toString(), "currentWeather");
        QVERIFY(QMetaObject::invokeMethod(applyButton, "clicked"));
        QVERIFY(QMetaObject::invokeMethod(removeButton, "clicked"));
        QVERIFY(document.hasPreview());
        QVERIFY(document.previewSummary().contains("Удалить"));
        QVERIFY(QMetaObject::invokeMethod(applyButton, "clicked"));
        QCOMPARE(document.widgets().size(), 4);
        // Removing every widget must leave an editable empty application.
        auto empty = QJsonDocument::fromJson(document.documentJson().toUtf8()).object();
        empty.insert("widgets", QJsonArray{});
        QVERIFY(document.previewDocument(json(empty)));
        QVERIFY(document.applyPreview());
        QTRY_COMPARE(repeater->property("count").toInt(), 0);
        QTest::qWait(30);
        auto *addButton = window->findChild<QObject *>("addWidget");
        QVERIFY(addButton);
        QVERIFY(QMetaObject::invokeMethod(addButton, "clicked"));
        QVERIFY(document.hasPreview());
        QTRY_COMPARE(repeater->property("count").toInt(), 1);
        auto *cancelButton = window->findChild<QObject *>("cancelMutation");
        QVERIFY(cancelButton);
        QVERIFY(QMetaObject::invokeMethod(cancelButton, "clicked"));
        QVERIFY(!document.hasPreview());
        // Optional visual evidence uses this isolated test document only.
        const auto capture = qEnvironmentVariable("MUTYVA_TEST_CAPTURE");
        if (!capture.isEmpty()) {
            QVERIFY(document.resetDemo());
            QVERIFY(document.applyPreview());
            QVERIFY(document.addHumidityCard());
            window->resize(1020, 1200);
            QTest::qWait(200);
            const auto image = window->grabWindow();
            QVERIFY(!image.isNull());
            QVERIFY(image.save(capture));
        }
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join('\n')));
    }
};

QTEST_MAIN(RuntimeTest)
#include "RuntimeTest.moc"
