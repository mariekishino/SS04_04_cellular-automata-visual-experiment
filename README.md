# SS04_04_cellular-automata-visual-experiment

セルオートマトンから発想した人工生命を画面に描く、C++ の視覚実験です。
無音でも自律的に動く「身体」を作り、そこに音を環境入力として与え、刺激の履歴が残るかを調べます。

## 3行で言うと

- **何を作るか:** まとまりを保ちながら流体のように動く視覚的存在。音はスイッチではなく環境入力。
- **今どこか:** **Phase 0(モデル探索)**。コードはまだなく、ドキュメントだけがあります。
- **次に何をするか:** Lenia 型と Gray–Scott 型の 2 モデルを小さい格子で動かし、目と数値で比較する。

## 現在のフェーズ

**Phase 0 — モデル探索** → [docs/phases/00_model_exploration.md](docs/phases/00_model_exploration.md)

フェーズを進めるときは、この節だけを書き換えます。コーディングエージェントはこの節を「今の作業範囲」として読みます([CLAUDE.md](CLAUDE.md) 参照)。

## ビルドと実行

未実装。Phase 0 の実装時に、動作確認した OS・コンパイラ・依存ライブラリの版と一緒にここへ記載します。

## ドキュメントの地図

各ファイルの冒頭に「読者」と「役割」を書いてあります。迷ったらこの表から入ってください。

| 読者 | ファイル | 内容 |
| --- | --- | --- |
| 全員 | [docs/00_vision.md](docs/00_vision.md) | 何を作りたいか、何を問いたいか |
| 全員 | [docs/03_roadmap.md](docs/03_roadmap.md) | Phase 0〜7 の全体像 |
| 本人 | [docs/04_development_workflow.md](docs/04_development_workflow.md) | 1 フェーズをどう回すか(人間側の手順) |
| 実装者 | [docs/01_architecture.md](docs/01_architecture.md) | 技術方針、責務の境界、性能の考え方 |
| 実装者 | [docs/02_experiment_protocol.md](docs/02_experiment_protocol.md) | 何を記録し、何を測るか |
| 実装者 | [docs/phases/](docs/phases/) | 各フェーズの具体的な実装範囲と完了条件 |
| 実装者 | [docs/decisions/](docs/decisions/) | 後から変えるときに理由が要る決定 |
| 本人 | [docs/learning/](docs/learning/) | C++ とモデルの学習メモ |
| 本人 | [docs/future/](docs/future/) | 今は範囲外の将来構想 |
| エージェント | [CLAUDE.md](CLAUDE.md) | 常時守るルール(自動で読み込まれる) |
| エージェント | [.claude/skills/phase-report/SKILL.md](.claude/skills/phase-report/SKILL.md) | フェーズ完了時の報告様式(`/phase-report`) |

「実装者」は本人とコーディングエージェントの両方を指します。
