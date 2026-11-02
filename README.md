# crane_x7_simple_examples [![build-test](https://github.com/cit22ros2/crane_x7_simple_examples/actions/workflows/test.yaml/badge.svg)](https://github.com/cit22ros2/crane_x7_simple_examples/actions/workflows/test.yaml)
このパッケージは, ロボットアーム（CRANE-X7）単独でのピッキングとRGBDセンサ（RealsenseD435）を併用したピッキングの２つだけをまとめたシンプルなROS 2パッケージです. 講義の参加者が, このパッケージをひな形にして製作を行うために作成されました.   


## このパッケージを使う前に
### ROS 2及びCRANE-X7セットアップ  
  **※この資料はUbuntu 22.04 LTS, ROS 2 humbleの環境で書いています.**
  
  * ROS 2インストール  
　　上田先生の[動画](https://youtu.be/mBhtD08f5KY)及び[インストールスクリプト](https://github.com/ryuichiueda/ros2_setup_scripts)を参照し, インストールを行ってください.   
  * CRANE-X7及び関連パッケージのインストール  
　　[RT社公式リポジトリ](https://github.com/rt-net/crane_x7_ros/tree/ros2)よりインストールできます. 以下にインストールコマンドを載せます.   
    ```
    # Setup ROS environment
    source /opt/ros/humble/setup.bash

    # Download crane_x7 repositories
    mkdir -p ~/ros2_ws/src
    cd ~/ros2_ws/src
    git clone -b humble https://github.com/rt-net/crane_x7_ros.git
    git clone -b humble https://github.com/rt-net/crane_x7_description.git

    # Install dependencies
    rosdep install -r -y -i --from-paths .

    # Build & Install
    cd ~/ros2_ws
    colcon build --symlink-install
    source ~/ros2_ws/install/setup.bash
    ```
    （[https://github.com/rt-net/crane_x7_ros/tree/ros2/README.md](https://github.com/rt-net/crane_x7_ros/tree/ros2/README.md)より転載）  
    (#の行はコメント, ~~$の行はshellのコマンドです~~)  
    また, インストールが完了したらパッケージに含まれるサンプルコードをシミュレータ（Gazebo）で試すことができます.  
    詳しくは[こちら](https://github.com/rt-net/crane_x7_ros/tree/ros2/crane_x7_examples)を参照してください. 
  * USBポートの設定（実機のCRANE-X7を動かす際に必要となります）
	  ```
	  # 一時的な付与の場合(上手くいかない時はUSBポートの名前を確認してください)
	  sudo chmod 666 /dev/ttyUSB0
	
	  # 永続的な付与の場合(再起動を伴います)
    sudo usermod -aG dialout $USER
	  reboot
	  ```
  >   crane_x7_controlの[README](https://github.com/rt-net/crane_x7_ros/blob/ros2/crane_x7_control/README.md)に詳しく書いてあります. 

### RealSenseセットアップ
[IntelRealSenseのgithub
](https://github.com/IntelRealSense/librealsense/blob/development/doc/distribution_linux.md#installing-the-packages)を参照してください. 以下[先ほどのページ](https://github.com/IntelRealSense/librealsense/blob/development/doc/distribution_linux.md#installing-the-packages)から引用
```
# Register the server's public key:
sudo mkdir -p /etc/apt/keyrings
curl -sSf https://librealsense.intel.com/Debian/librealsense.pgp | sudo tee /etc/apt/keyrings/librealsense.pgp > /dev/null

# Make sure apt HTTPS support is installed:
sudo apt-get install apt-transport-https

# Add the server to the list of repositories:
echo "deb [signed-by=/etc/apt/keyrings/librealsense.pgp] https://librealsense.intel.com/Debian/apt-repo `lsb_release -cs` main" | \
sudo tee /etc/apt/sources.list.d/librealsense.list
sudo apt-get update

# Install the libraries (see section below if upgrading packages):
sudo apt-get install librealsense2-dkms
sudo apt-get install librealsense2-utils

# セットアップできているか確認（ビューワーの起動）:  
realsense-viewer

```

# このパッケージの使い方
## インストール
```
cd ~/ros2_ws/src
git clone https://github.com/CIT-Autonomous-Robot-Lab/crane_x7_simple_examples.git 
```
※ただし，このパッケージはこのコードを雛形にして講義のパッケージを作成してほしいという意図の元作成されたので，
このリポジトリをforkしてそちらをcloneすることを推奨します．
## ビルド 
```
cd ~/ros2_ws
colcon build
source ~/ros2_ws/install/setup.bash

# ３行目のコマンドは、~/.bashrcに書いておくことを推奨します.   
# 下のコマンドで.bashrcに追記できます.  
echo 'source ~/ros2_ws/install/setup.bash' >> ~/.bachrc
# .bashrcに書いてあるとき下のコマンドが代わりにできます.
source ~/.bashrc
```
## 実行  
シミュレータ（Gazebo）あるいは実機で動かす際には、
実機orシミュレータの立ち上げのコマンドと動作を実施するコマンドの2つを起動する必要があります. 
詳しくは[こちら](https://github.com/rt-net/crane_x7_ros/tree/ros2/crane_x7_examples#3-move_group%E3%81%A8controller%E3%82%92%E8%B5%B7%E5%8B%95%E3%81%99%E3%82%8B)
を確認してください.   

### pick_and_move
ピッキング（特定の場所にある物体を掴む・持ち上げる・運ぶ・置く）を行うコードです.   
次のコマンドで実行できます.  

* for Gazebo

  **terminal 1:** `ros2 launch crane_x7_gazebo crane_x7_with_table.launch.py `

  **terminal 2:** `ros2 launch crane_x7_liner_move pick_and_move.launch.py use_sim_time:=true`

* for real machine

  **terminal 1:** `ros2 launch crane_x7_examples demo.launch.py port_name:=/dev/ttyUSB0`

  **terminal 2:** `ros2 launch crane_x7_liner_move pick_and_move.launch.py`

![pick_and_moveのデモ動画](https://github.com/cit22ros2/crane_x7_simple_examples/assets/79034190/1ae6a7f3-ab86-4d11-852e-52d320dc2758)


### camera_picking
RGBDセンサを用いたピッキング（特定の色の物体を検知する・掴む・持ち上げる・運ぶ・置く）コードです.   
このコードでは、青い物体の位置を検知し物体を掴みます.   
Realsenseを接続し次のコマンドで実行できます.  

* for real machine

  **terminal 1:** `ros2 launch crane_x7_examples demo.launch.py port_name:=/dev/ttyUSB0 use_d435:=true`

  **terminal 2:** `ros2 launch crane_x7_liner_move camera_picking.launch.py`
  
![camera_pickingのデモ動画](https://github.com/cit22ros2/crane_x7_simple_examples/assets/79034190/6f7d34e0-7bf0-4dd0-a9cf-9fd8b9e58e62)


# 引継ぎ事項
## よく起きるエラーと解決法
* 止め方がわからない  
    `Ctrl + C`で止めることができます. 
* GazeboでCRANE-X7が動かない/動きが途中で止まる.   
    実行時に`use_sim_time:=true`が抜けていることが多いです. 
* RViz/Gazeboの画面が真っ黒  
    1度止めてもう一度立ち上げると大体解決します. 
* `colcon build`が通らない.   
    1. package名やコードの名前は統一されていますか?  
    2. 依存関係の問題かもしれません次を試してみましょう. 
    ```
    rosdep update
    rosdep install -r -y --from-paths --ignore-src ./
    ```
* CRANE-X7のROS 2対応のリポジトリが見つからない.   
    ブランチを変えるとあります. 
* 実行コマンドを叩いたのにパッケージが見つからないと出る.  
    実行コマンドで呼び出す名前とパッケージのものの名前が間違ってる可能性があります.
* なぜかアームの動きがスキップされる  
    MoveIt 2の制限に引っかかっている可能性があります．制限を緩和すると動きます．（ただし，ダイナミックに動くので周囲への注意が必要） 

## やり残したこと
* Gazeboに自分で作成したモデルの導入
* ~~Gazebo上でのRealSenseの動作~~ずっと前にkuwamaiさんがやっておられました．[こちら](https://github.com/Kuwamai/crane_x7_d435)
	* ROS 2への移植
* ~~Open CVのプログラム([color_detection.cpp](https://github.com/cit22ros2/crane_x7_simple_examples/blob/main/src/color_detection.cpp))の変更~~
  * ~~見つける色の変更~~
  * ~~探すのに動きを組み合わせるなど~~
* 姿勢を維持したまま移動すること


# Thankyou for using
* 2024
  * [骨格解析を用いたマニピュレータ操作](https://github.com/akajaika/crane_x7_Hand_Tracking)
  * [全自動じゃんけん機](https://github.com/uchiyamashouki/sazaesann)
  * [ハンコを押すマニピュレータ](https://github.com/ken222d/crane_x7_stamp)
  * [回転寿司自動配膳アーム](https://github.com/bloodlemon2/crane_x7_serving_food_examples?tab=readme-ov-file)

# ライセンス
(C) 2023 Keitaro Nakamura, Ryotaro Karikomi

このパッケージはRT Corporationの公開する[パッケージ](https://github.com/rt-net/crane_x7_ros/tree/ros2)に含まれる[crane_x7_examples](https://github.com/rt-net/crane_x7_ros/tree/ros2/crane_x7_examples)を改変したものです．
具体的には，
* launch/camera_picking.launch.py
* launch/pick_and_mvoe.launch.py
* src/pick_and_move_tf.py
* src/color_detection.py
* CMakeList.txt
* package.xml

に微小な改変を
* src/pick_and_move.cpp

に大幅な改変を加え作成されました.  
このパッケージはApache License, Version 2.0に基づき公開されています.  
ライセンスの全文は[LICENSE](./LICENSE)から確認できます.  
