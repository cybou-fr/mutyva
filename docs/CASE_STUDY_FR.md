# Étude de cas — MUTYVA

**Projet personnel open source — C++20, Qt/QML, architecture applicative et ingénierie assistée par IA.**  
**État :** prototype évolutif ; tests d’intégration ChatGPT documentés sur Windows, sans qualification de production ou multiplateforme complète.

## 1. Problème et objectif

Comment modifier une application par une demande en langage naturel sans exécuter arbitrairement le code généré par une IA ? MUTYVA vise un environnement déclaratif local, dont l’interface peut évoluer sans compromettre la maîtrise de ses composants et de son état.

## 2. Architecture retenue

- **C++20 / Qt / QML :** moteur natif et présentation graphique.
- **AppDocument et schémas :** descriptions JSON validées des composants et de leur configuration.
- **ComponentRegistry :** palette de composants connus, rendus avec des types QML préconstruits ; aucune évaluation libre de QML produit par le modèle.
- **MutationEngine :** contrôle du schéma et de la sémantique, prévisualisation et application sur confirmation.
- **État local :** générations de configuration, données des formulaires et listes SQLite, indépendantes de l’IA.
- **ChatGPT :** connexion et traitement des réponses isolés de l’exécution normale de l’application.

**Flux :** demande → proposition IA → validation → aperçu → confirmation → génération enregistrée → rendu Qt.

[Architecture détaillée](02_ARCHITECTURE.md) · [Décisions](09_ENGINEERING_DECISIONS.md)

## 3. Réalisation observable

Le code comprend un éditeur Qt, un moteur de mutations contrôlées, des exemples déclaratifs, une démonstration météo Open-Meteo, des mécanismes de persistance et des tests Qt. Le projet documente un test Windows du 19 octobre 2026 intégrant une réponse de ChatGPT, une mutation et la restauration d’une génération. Cette preuve est issue du développement, non d’un audit indépendant.

[Tests](../tests/RuntimeTest.cpp) · [Intégration IA et limites](15_AI_CONNECTION.md) · [Scénarios réels](16_LIVE_AI_TESTS.md)

## 4. Choix et compromis

| Décision | Objectif | Limite |
| --- | --- | --- |
| Déclarations contrôlées plutôt que code généré exécutable | Garder des actions vérifiables | Palette de composants bornée |
| Prévisualisation avant application | Maintenir une validation humaine | Étape supplémentaire dans le parcours |
| Exécution locale des applications sauvegardées | Fonctionner sans appel IA | Nouvelles mutations nécessitant une connexion |
| Aucun backend ou marketplace MUTYVA | Réduire l’infrastructure | Pas de synchronisation communautaire |

## 5. Validation et limites

Le MVP complet, les sessions persistantes protégées, les connecteurs génériques et la qualification de Windows, macOS, Linux, Android et iOS restent distincts des fonctions déjà réalisées. Les tests publiés ne sont pas une garantie de sécurité de production.

## 6. Compétences illustrées

Architecture logicielle, C++/Qt/QML, conception déclarative, intégration contrôlée de l’IA, documentation, gestion d’état, prototypage et réalisation d’un MVP. Il s’agit d’un projet personnel, pas d’une mission client.

[Retour au dépôt](../README.md)
