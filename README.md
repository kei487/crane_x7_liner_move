# crane_x7_liner_move

CRANE-X7 の手先を **直線移動** させるための ROS 2 パッケージです。  
MoveIt の `setPoseTarget()` だけでは途中経路が関節空間で決まり、手先が大きく回ることがあるため、`computeCartesianPath` による直線補間を使います。

対応環境: **Ubuntu 22.04 / ROS 2 Humble**

## できること

| 機能 | 実行ファイル / launch | 概要 |
|---|---|---|
| 直線移動サービス | `linear_move_server` | 目標座標 `(x, y, z)` を受け取り、移動成否を返す |
| サービス呼び出しデモ | `move_tip_straight_demo` | サービスを連続で叩くデモ |
| 固定軌道のピッキング | `pick_and_move` | 接近→把持→持ち上げ→配置を直線移動で実行 |
| カメラ連動ピッキング | `pick_and_move_tf` + `color_detection` | 色検出した物体を掴む |

## 前提セットアップ

### CRANE-X7（公式パッケージ）

[rt-net/crane_x7_ros](https://github.com/rt-net/crane_x7_ros/tree/ros2) をワークスペースに入れてビルドしてください。

```bash
source /opt/ros/humble/setup.bash
mkdir -p ~/cranex7_ws/src
cd ~/cranex7_ws/src
git clone -b humble https://github.com/rt-net/crane_x7_ros.git
git clone -b humble https://github.com/rt-net/crane_x7_description.git
cd ~/cranex7_ws
rosdep install -r -y -i --from-paths src
colcon build --symlink-install
source ~/cranex7_ws/install/setup.bash
```

実機を使う場合は USB 権限が必要です。

```bash
sudo chmod 666 /dev/ttyUSB0
# 永続化する場合
sudo usermod -aG dialout $USER
```

詳細: [crane_x7_control README](https://github.com/rt-net/crane_x7_ros/blob/ros2/crane_x7_control/README.md)

### このパッケージのビルド

```bash
cd ~/cranex7_ws/src
git clone https://github.com/kei487/crane_x7_liner_move.git
cd ~/cranex7_ws
colcon build --packages-select crane_x7_liner_move
source ~/cranex7_ws/install/setup.bash
```

## 起動の基本

動作ノードを起動する前に、**move_group / コントローラ** を先に起動します。

* Gazebo  
  `ros2 launch crane_x7_gazebo crane_x7_with_table.launch.py`
* 実機  
  `ros2 launch crane_x7_examples demo.launch.py port_name:=/dev/ttyUSB0`

Gazebo 利用時は、後続の launch / run に必ず `use_sim_time:=true` を付けてください。  
付け忘れると時刻が合わず、現在姿勢の取得に失敗して止まります。

---

## 1. 直線移動サービス（推奨）

手先目標を入力し、直線移動の成否を出力する ROS 2 サービスです。

* サービス名: `/move_tip_straight`
* 型: `crane_x7_liner_move/srv/MoveTipStraight`
* 入力: `x, y, z`（任意で姿勢 `use_custom_orientation` + RPY[deg]）
* 出力: `success`, `message`, `fraction`

姿勢を指定しない場合（`use_custom_orientation: false`）は、把持姿勢 RPY=`(90, 0, 90)` [deg] を使います。

### サーバ起動

```bash
# 実機
ros2 launch crane_x7_liner_move linear_move_server.launch.py

# Gazebo
ros2 launch crane_x7_liner_move linear_move_server.launch.py use_sim_time:=true
```

### デモクライアント

```bash
# 組み込み軌跡（接近 → 把持 → 持ち上げ → 配置）
ros2 run crane_x7_liner_move move_tip_straight_demo

# 単発で目標座標を指定
ros2 run crane_x7_liner_move move_tip_straight_demo -- 0.25 -0.15 0.10
```

### CLI から直接呼ぶ

```bash
ros2 service call /move_tip_straight crane_x7_liner_move/srv/MoveTipStraight \
  "{x: 0.25, y: -0.15, z: 0.10, use_custom_orientation: false}"
```

直線が途中で切れた場合は、高い経由点を挟んで再試行します。それでも失敗すると `success: false` を返します。

---

## 2. pick_and_move（固定ピッキング）

固定座標への接近・把持・運搬を直線移動で行います。

```bash
# Gazebo
ros2 launch crane_x7_gazebo crane_x7_with_table.launch.py
ros2 launch crane_x7_liner_move pick_and_move.launch.py use_sim_time:=true

# 実機
ros2 launch crane_x7_examples demo.launch.py port_name:=/dev/ttyUSB0
ros2 launch crane_x7_liner_move pick_and_move.launch.py
```

---

## 3. camera_picking（色検出ピッキング）

RealSense D435 で青い物体を検出し、TF 経由で把持します。  
RealSense のセットアップは [librealsense の手順](https://github.com/IntelRealSense/librealsense/blob/development/doc/distribution_linux.md#installing-the-packages) を参照してください。

```bash
ros2 launch crane_x7_examples demo.launch.py port_name:=/dev/ttyUSB0 use_d435:=true
ros2 launch crane_x7_liner_move camera_picking.launch.py
```

---

## トラブルシューティング

| 症状 | 対処 |
|---|---|
| Gazebo で動かない / 途中で止まる | `use_sim_time:=true` が付いているか確認 |
| `Failed to fetch current robot state` | 同上。Gazebo 起動後にサーバを再起動 |
| グリッパ閉じが失敗する | 物体接触で目標角度に届かないことがある。サービス側の把持シーケンスでは続行する実装になっている |
| 直線移動の `fraction` が低い | 目標が遠すぎる・机と干渉・特異点付近の可能性。経由点を使うか、目標座標を見直す |
| パッケージが見つからない | `source ~/cranex7_ws/install/setup.bash` とパッケージ名 `crane_x7_liner_move` を確認 |
| ビルド失敗 | `rosdep install -r -y -i --from-paths src` のあと再ビルド |

停止は `Ctrl + C` です。

## ライセンス

Apache License 2.0（詳細は [LICENSE](./LICENSE)）。  
本パッケージは [crane_x7_examples](https://github.com/rt-net/crane_x7_ros/tree/ros2/crane_x7_examples) を改変・拡張したものです。
