# 0003 — SDL3 の窓表示を CMake オプションで追加し、仮想ディスプレイで検証する

**読者:** 実装者(本人とコーディングエージェント)。
**日付:** 2026-09-24
**状態:** 採用(Phase 1)

## 問い

決定 0002 で Phase 0 は PNG 出力にした。Phase 1 の「長く観察する単体アプリ」に窓表示を付けるとき、実装環境(exe.dev VM、表示先なし)と本人の確認環境(Windows PC または Mac)の違いをどう扱うか。

## 決定

- `cave_window` を CMake オプション `CAVE_WITH_SDL3`(既定 OFF)で追加する。既定 OFF なので、Phase 0 の CLI とテストは SDL3 なしでビルドできる。
- システムに SDL3 が見つからなければ、CMake の FetchContent で `release-3.4.16` を取得して静的ビルドする。Phase 1 では video 以外(audio、camera、gpu、haptic、joystick、sensor、hidapi)を OFF にしてビルド時間を減らす。audio は Phase 2 で ON に戻す。
- VM では Xvfb(仮想 X サーバー)上で起動し、`--frames` / `--keys` / `--shot-every` で操作を自動注入して、キー処理とスクリーンショット保存を検証する。X サーバー側からも `xwininfo` で窓の存在、`import` で画面を記録する。
- 実機の窓(Windows / Mac)は本人が確認する。エージェントは「仮想ディスプレイで検証済み、実機は未確認」と報告する。

## 理由

- Ubuntu 24.04 の apt に SDL3 はない。FetchContent なら本人の Mac や Windows でも同じ CMake で通る(要確認)。
- 描画フレーム数とシミュレーションの固定時間刻みの分離は、実際に窓を開いて時間を計らないと検証できない。Xvfb では vsync がなく約 300 fps で描画されるが、steps/s は指定どおり 60 のままだった。この分離が動いている証拠になる。
- キー操作を SDL のイベントキューに注入する方式なら、実際のキー処理と同じコードパスを自動で通せる。

## 見送った案

- SDL2 を apt で入れる: 0001 で SDL3 を候補にしており、API が異なるので二度手間になる。
- 窓表示を Phase 2 まで先送りする: Phase 1 の要件(停止・再開・リセットを本人が操作して観察する)を満たせない。
- ブラウザ表示(WebAssembly): 将来の可能性としては残るが、Phase 1 の範囲を超える。

## 再検討する条件

- 本人の環境で FetchContent のビルドが通らない場合は、`brew install sdl3` などシステムの SDL3 を `find_package` で使う経路を先に試し、それでも駄目なら記録する。
- Phase 2 で音声を入れるとき、SDL3 の audio サブシステムを ON にするか、別ライブラリにするかを決める。

## 検証記録(2026-09-24、VM)

- ビルド依存(Ubuntu 24.04): `libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxss-dev libxtst-dev libxinerama-dev libxkbcommon-dev libgl1-mesa-dev libegl1-mesa-dev libdbus-1-dev libudev-dev libdrm-dev libgbm-dev libwayland-dev libdecor-0-dev libibus-1.0-dev`。`libxtst-dev` がないと configure が XTEST で止まる。
- 起動時の出力: `SDL 3.4.16  video driver: x11  renderer: opengl  window 512x512`、X11 の窓 ID あり。
- `xwininfo -root -tree` に `cave_window` が現れ、タイトルバーに `cave  RUN  step 120  t=12.0  60 steps/s  mass 71.4  comp 2` が出た。
- `--keys "90:space,120:n,121:n,122:n,150:space,240:r,300:s,330:plus,360:minus,410:q"` で、停止 → 1 ステップ ×3 → 再開 → リセット(ステップ数が 0 から再開) → スクリーンショット → 速度 2 倍 → 半分 → 終了、の順に動作した。

## 実機での確認(2026-09-24、本人)

Mac でリポジトリを clone し、`phase-1-autonomous-body` を `CAVE_WITH_SDL3=ON` でビルドして `cave_window` を起動。窓が開き、Orbium の動きを目視で確認した。使用した SDL3 の入手経路(brew か FetchContent か)と macOS の版は未記録。
