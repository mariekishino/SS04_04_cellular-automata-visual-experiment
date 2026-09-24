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

動作確認した環境: Ubuntu 24.04 (exe.dev VM, x86_64, 2 コア)、g++ 13.3.0、CMake 3.28.3。依存ライブラリはありません。動画とシート画像の生成にだけ ffmpeg 6.1 と ImageMagick(`montage`)を使います。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
(cd build && ctest --output-on-failure)

./build/cave --list                     # プリセットと初期条件の一覧
./build/cave --model lenia --preset orbium --init orbium --width 64 --height 64 --steps 1000 --every 10 --out experiments/out/L1
./build/cave --model grayscott --preset coral --init square --width 128 --height 128 --steps 10000 --every 100 --out experiments/out/G1
scripts/make_media.sh experiments/out/L1    # video.mp4 と sheet.png を作る
```

出力先には `config.json`(再現に必要な設定)、`metrics.csv`、`frames/*.png`、`state.bin`(再開用)、`summary.txt`(健全性フラグと計測時間)が書かれます。Phase 0 では窓を開かず画像に書き出します([decisions/0002](docs/decisions/0002_headless_rendering_phase0.md))。

## ドキュメントの地図

各ファイルの冒頭に「読者」と「役割」を書いてあります。迷ったらこの表から入ってください。

| 読者 | ファイル | 内容 |
| --- | --- | --- |
| 全員 | [docs/00_vision.md](docs/00_vision.md) | 何を作りたいか、何を問いたいか |
| 全員 | [docs/03_roadmap.md](docs/03_roadmap.md) | Phase 0〜7 の全体像 |
| 本人 | [docs/04_development_workflow.md](docs/04_development_workflow.md) | 1 フェーズをどう回すか(人間側の手順) |
| 実装者 | [docs/01_architecture.md](docs/01_architecture.md) | 技術方針、責務の境界、性能の考え方 |
| 実装者 | [docs/02_experiment_protocol.md](docs/02_experiment_protocol.md) | 何を記録し、何を測るか |
| 実装者 | [docs/phases/](docs/phases/) | 各フェーズの具体的な実装範囲と完了条件。Phase 0 の報告は [00_report.md](docs/phases/00_report.md) |
| 実装者 | [docs/decisions/](docs/decisions/) | 後から変えるときに理由が要る決定 |
| 本人 | [docs/learning/](docs/learning/) | C++ とモデルの学習メモ |
| 本人 | [docs/future/](docs/future/) | 今は範囲外の将来構想 |
| エージェント | [CLAUDE.md](CLAUDE.md) | 常時守るルール(自動で読み込まれる) |
| エージェント | [.claude/skills/phase-report/SKILL.md](.claude/skills/phase-report/SKILL.md) | フェーズ完了時の報告様式(`/phase-report`) |

「実装者」は本人とコーディングエージェントの両方を指します。

## 参考にしたもの

コードはどのリポジトリからもコピーしていません。式・定数・データの出典は以下のとおりで、各ソースファイルの冒頭コメントにも同じ内容を書いてあります。

| 対象 | 出典 | 使った範囲 |
| --- | --- | --- |
| Lenia 型の更新則 | B. W.-C. Chan, "Lenia: Biology of Artificial Life", Complex Systems 28(3), 2019 ([arXiv:1812.05433](https://arxiv.org/abs/1812.05433)) | カーネル・成長関数・更新式の定義。単一チャンネル・単一リング・直接畳み込みに単純化 |
| Orbium の初期配置とパラメータ | [Chakazul/Lenia](https://github.com/Chakazul/Lenia)(MIT License、(c) Bert Wang-Chak Chan)の `Python/animals.json` | 20×20 のセル値と R, T, m, s をデコードして [src/core/orbium_cells.hpp](src/core/orbium_cells.hpp) に同梱 |
| Gray–Scott の式 | J. E. Pearson, "Complex Patterns in a Simple System", Science 261 (1993) 189–192 | 反応拡散方程式 |
| Gray–Scott の離散化と定数 | [Karl Sims, Reaction-Diffusion Tutorial](https://www.karlsims.com/rd.html) | 9 点ラプラシアンの重み、Du, Dv, dt、coral / mitosis の F, k |

Grid、指標、PNG 出力、CLI、テスト、実験の設計はこのリポジトリで書いたものです。

## ライセンス

[MIT License](LICENSE)。同梱している Orbium のデータは Chakazul/Lenia 由来で、同じく MIT です。
