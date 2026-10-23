# Étude de cas — MUTYVA, architecture native pilotée par contrats

**Projet personnel open source · Architecture logicielle et système · C++20 · Qt 6.12.x / QML · SQLite · IA assistée**  
**État au 23 octobre 2026 :** prototype local, développement en cours. Le CI de vérification statique des contrats est vert; la compilation/testation Windows sous **Qt 6.12** demeure bloquée par la configuration des secrets du Qt Online Installer. Des exécutions Windows sous Qt 6.8 constituent des preuves historiques, **pas une qualification Qt 6.12, mobile ou production**.

## Le problème de conception

Permettre à une personne de créer ou d'adapter une application native par une demande en langage naturel, **sans faire exécuter le code produit par le modèle** et sans faire dépendre le fonctionnement ultérieur de l'application d'un appel à l'IA. Une mutation de l'interface ne doit ni détruire les données enregistrées, ni élargir silencieusement les permissions.

**Contraintes structurantes :** exécution locale, pas de backend ou comptes MUTYVA, pas de marketplace/synchronisation, pas de plugins exécutables arbitraires, composants natifs Qt, compatibilité des données, validation et consentement avant application. L'accès à ChatGPT concerne uniquement la phase facultative de conception, pas l'exécution locale.

## Mon approche d'architecture

```mermaid
flowchart LR
  R[Usage réel + exigences FR/NFR/ASR] --> T[Contraintes et modèles de menace]
  T --> O[Options et arbitrages]
  O --> A[Décision ADR + contrats]
  A --> F[Tests négatifs / preuve de faisabilité]
  F --> I[Tranche verticale Qt]
  I --> V[CI et preuves observables]
  V --> E[Réévaluation + roadmap]
```

La méthode documentée est **besoin → exigences significatives → alternatives → décision → contrat → test → implémentation → vérification → évolution**, et non « demander à l'IA d'écrire du code puis réorganiser le tout ». Les hypothèses et limites sont explicites. La réalisation assistée par IA ne dispense pas de maîtriser les frontières de sécurité, les données et les critères d'acceptation.

## Décisions et compromis visibles

| Problème | Décision | Alternative écartée et raison |
|---|---|---|
| Déploiement et exécution native | **Monolithe modulaire local Qt/C++20** | Microservices/backend : charges opérationnelles, identité et synchronisation sans besoin utilisateur |
| Production assistée par l'IA | **Déclarations JSON validées et composants QML précompilés** | QML/JS/SQL générés et exécutés : surface de risque et garanties faibles |
| Mutation et restauration | **Générations de définition immuables, métadonnées vérifiées, Apply explicite** | Fichier mutable unique : perte de continuité et récupération complexe |
| Données utilisateur | **SQLite indépendant des versions de l'interface** | Enregistrer les lignes dans chaque snapshot UI : retour arrière destructeur |
| Composition future | **Sélecteurs typés, événements et commandes contrôlés par l'hôte** | Moteur générique de scripts/règles : coût et risque disproportionnés |
| Données externes | **Adaptateurs HTTPS publics approuvés, lecture seule en première étape** | URLs et accès arbitraires : SSRF, fuites, credentials, écritures distantes |

Voir le [registre d'ADR](adr/README.md) et l'[architecture actuelle et ses risques](23_ARCHITECTURE_ASSESSMENT.md).

## Architecture actuelle : des responsabilités traçables

- `DeclarativeRuntime` : validation du schéma v2, références, permissions, mutations.
- `AppDocument` : génération confirmée, historique parent/hachage, prévisualisation.
- `DesignerContext` : palette réelle de 21 types, schémas et recettes d'assemblage transmises à ChatGPT, **sans données enregistrées**.
- `InteractionState` : état UI, sélection et opérations; découpage de ses responsabilités prévu avant le moteur de liaisons génériques.
- `LocalDataStore` / `RecordStore` : données SQLite locales, CRUD transactionnel, révisions.
- `WeatherService` : source HTTPS Open-Meteo codée et autorisée; connecteurs génériques non livrés.
- `AiChat` / `ChatSession` : communication ChatGPT séparée de l'exécution normale.
- QML de confiance : UI native issue des composants précompilés, **pas de compilation de code IA**.

**Exemple vérifiable :** les composants `record-form` et `record-list` partagent un modèle de tâches et stockent de vraies lignes; les filtres/sorts sont déclaratifs. **Limites actuelles :** pas de liaison générique `select→record-list.filter`, pas de multi-app indépendante, pas de restauration guidée de toutes les générations ni de moteur REST arbitraire.

## Exigences de qualité, risques et preuves

Le [rapport ASR et risques](23_ARCHITECTURE_ASSESSMENT.md) relie intégrité de l'historique, protection de la vie privée, isolation des capacités, continuité des données, réactivité UI, adaptabilité et reproductibilité à des contrôles mesurables.

**Preuves disponibles :** [CI statique des contrats](https://github.com/cybou-fr/mutyva/actions/runs/39955309544) vert (catalogue/schémas/exemples); [ancien CI Windows Qt 6.8](https://github.com/cybou-fr/mutyva/actions/runs/39933293033) avec 65 tests réussis et 21 images générées. **Non prouvé** à cette date : Qt 6.12 Windows, Android, restauration complète et création par IA d'une application locale indépendante.

**Défauts ouverts et assumés :** absence de HEAD avec snapshots existants, différences sémantiques incomplètes dans la prévisualisation, redondance des fonctions de `InteractionState`, normalisation partielle de sélection d'enregistrements, plafonds/délais réseau du service météo. Ces problèmes ont des scénarios d'échec et des critères de résolution dans la roadmap : ils ne sont pas cachés derrière une démonstration.

## Pourquoi ce projet est un exemple d'expertise architecturale

MUTYVA illustre : **analyse de besoins et contraintes, ADR avec alternatives, architecture native, frontières de confiance, contrats et compatibilité, données transactionnelles, conception de composants, stratégie de tests et de migration, CI et pilotage par preuves**. L'enjeu n'est pas le nombre de classes ou de widgets : c'est la maîtrise des changements avec un coût technique raisonnable.

Le [workflow d'ingénierie](24_ARCHITECTURE_WORKFLOW.md) formalise une méthode réutilisable pour architecture, modernisation/migration de systèmes et livraison de MVP. Les évolutions futures ne sont validées qu'après preuve de fonctionnement et mise à jour des décisions.

**Nature du projet :** réalisation personnelle open source, pas une mission client ni une attestation d'audit externe.

[Source](../README.md) · [Architecture & ASR](23_ARCHITECTURE_ASSESSMENT.md) · [ADRs](adr/README.md) · [Workflow](24_ARCHITECTURE_WORKFLOW.md) · [Roadmap](06_IMPLEMENTATION_ROADMAP.md)
