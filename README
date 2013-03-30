>* この文書中の**私**とは、ActuaServerの開発者である阿久津鯖男(Sabao Akutsu)を指す。  
* この文書中の**ここ**とは、このファイルがあるリポジトリのルートを指す。  
* この文書中の**ActuaServer**とは、ここにあるActuaSvrフォルダ内の
ファイル群をコンパイル、リンクして生成されるプログラムに対して私が与えた名称である。

##最初に
 ActuaServerは**Arduino**で安価なアクチュエータ（RCサーボ、ステッピングモータ、DCモータなど）
 をリアルタイム制御するために製作いたしました。シリアル通信を利用して簡単なコマンドを送ることで
 複数のアクチュエータを同時に制御することが可能です。
 マルチタスク制御を実現する為、Quantum Leaps, LLC製QP frameworkのpreemptive QK kernelを採用しております。
 QP frameworkはGPLで一般に公開されており、ライセンス条項に従う限り誰でもこれを使用または利用することが出来ます。

##ActuaServer実行までの手順  
1. Arduino本体を入手します（もしくはそれに準ずる環境を用意して下さい）。
私はUNOでしか動作確認をしておりませんが、MEGAなどでも利用出来るかもしれません。  
2. Arduino公式サイトから開発環境(IDE)をダウンロードします.。
<center><http://www.arduino.cc/></center>
3. Arduino公式サイトからArduino用のQP frameworkをダウンロードします。
<center><http://playground.arduino.cc/Code/QP></center>
	ドキュメントに従ってArduino IDEのフォルダに必要なフォルダをコピーします。
4. フレームワークに付属のpdfなどを参考にしてpreemptive QK kernelを
 使用出来るようヘッダファイルを変更します。qp&#x5f;port.hの二ヶ所のマクロ定義のコメントをはずすだけのはずです。
 アップロードされているqp/libraries/qp/qp&#x5f;port.h はすでに変更済みです。
5. リポジトリをforkしてArduino IDE のsketchbookフォルダにActuaSvrフォルダをコピーします。
（またはsketchbookにActuaSvrへのリンクを置いてもよいと思います）
6. Arduino IDE でActuaSvrを開き、Arduino本体へプログラムをアップロードします。
 
##改変部分について 
ActuaServerに関する全てのソースコードはArduino用QP frameworkサンプルプログラム
 （DPP example, qp/examples/qp/qp&#x5f;dpp&#x5f;qk）に改変を加えて作成されました
 （実際に改変を行ったのはここに同梱されているものより以前のバージョンのものです）。 
 本来ならば元のファイル（qp&#x5f;dpp&#x5f;qkの中のファイル）のどこをどのように改変したのか
 明示するべきですが、 改変部分は多岐に及んでおり （元のファイル名も変更しております）
 それら全てを明示して公開するのはかえって混乱を招くかもしれません。
 オリジナルのサンプルとはほとんど別物になってしまっているからです。
 今回のリポジトリ公開にあたってはこうした改変部分の詳細は割愛致しました。何卒ご了承下さい。


>QP frameworkの各ファイルは私がサンプルをいじっている間に
バージョンアップされたようでActuaServerにはそれに対応するために
間に合わせの変更を加えてあります（namespaceの使用）。
今現在Quantum Leaps, LLCからQP frameworkを入手するとまたバージョンアップしているかも知れませんが、
少なくともここに同梱されているバージョンのQP frameworkであれば動作すると思います。
上記**ActuaServer実行までの手順**でQP frameworkを導入して
ActuaSvrのコンパイルエラーが起こる場合は、バージョンの違いが原因かも知れません。
その場合、ここにアップロードされているQP framework（qpフォルダ）を同じ手順で試してみて下さい。

##QP frameworkとActuaServerの著作権について
 QP frameworkそのものとそのサンプルプログラムの著作権はQuantum Leaps, LLCが有しています。
 QP frameworkの著作権とライセンスに関しての詳細はここにアップロードされた
 qp/copying.txtファイルをご覧下さい。
 上記の通りActuaServerはサンプルプログラムを改変して作られており、また各クラスは
 QP frameworkの機能と分かち難く結びついている事情から、各ソースファイルの冒頭に記した
 著作権者の表記は元の作成者であるQuantum Leaps, LLCとその改変者である私との併記と致しました。
 ActuaServerを構成する全てのソースファイルにおいて私が独自に改変、追加した部分に
 著作権が認められる部分があるならば私はその部分の著作権を主張します。
 また同様に将来私以外の誰かがActuaServerのソースコードを改変した場合、
 その部分に著作権が認められるのであれば改変部分の著作権はその改変者が有することになります。
 
##ActuaServerのライセンスについて
 ActuaServerを構成する各クラスはGPLv2の下で公開されたQP frameworkのクラスと
 "静的にリンク"しています。従って、ActuaServerを構成する全てのソースコードは
 GPLに基づいた管理を行う必要があります。
 Quantum Leaps, LLCが運営するサイトのアナウンスによれば、
 「このウェブサイトからダウンロード出来る全てのQP framework softwereは
 GPLv2またはそれ以降のパージョンでリリースできる」となっておりましたので、
 私は利便性を考え今回ActuaServerに**GPLv3**（GPLv2では改変した全てのファイルに
 その旨を記載する必要があるようです）を添付することと致しました。
 GPLv3の下であなたに与えられた自由の権利と義務に関する詳細は
 ここのルートにある**LICENSE**ファイルをお読み下さい。

##最後に
私のブログでActuaServerの詳細について解説しております。よろしければご訪問下さい。
<center><http://etweiz.blog.fc2.com/></center>
 
 <center>私以外の者がこの文書を改変することは禁ずる。
 					
 阿久津鯖男（Sabao Akutsu） Mar. 30, 2013</center>
