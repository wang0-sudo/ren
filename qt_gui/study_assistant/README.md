# Study Assistant

This folder contains a C++ study-question backend that now uses SQLite for local storage and also has a Qt desktop client.

Current features:
- SQLite `.db` storage
- normalized tag tables (`tags` + `question_tags`) on top of the `questions` table
- question CRUD
- optional image attachment path per question, with Qt-side preview
- tag and topic filtering
- simple spaced-review recommendation
- wrong-question book based on pending mistakes
- import and export for local backup (`.db` or legacy `.txt`, now with image path support in `V3`)
- per-question review statistics
- console demo for trying the service quickly
- Qt desktop window for browsing, filtering, editing, review marking, and import/export

Migration behavior:
- if `study_assistant_data.db` does not exist but `study_assistant_data.txt` exists, the demo imports the legacy text data automatically on first run

Run it with:
- `run_study_assistant.bat`
- `run_study_assistant_qt.bat`

Suggested next step:
- add charts or a dashboard page for daily review volume and weak-topic trends
- replace the compatibility `questions.tags` text column once you no longer need legacy migration/export support
