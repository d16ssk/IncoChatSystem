#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

#define CLIENT_NUM 8       // 同時に接続できるクライアント数
#define TEXT_BUF 512       // テキストのバッファ。クライアントと揃えておく。
#define NAME_SIZE 64       // 名前のデータサイズ。
#define PORT_NUM 8000      // ポート。クライアントと揃えておく。

// 名簿のクライアントの状態
enum state {
    empty,
    joining,
    standby,
};

// 名簿の情報
typedef struct client {
    int sockfd;
    char name[NAME_SIZE];
    struct sockaddr_in client_addr;
    enum state client_state;
} CLIENT;
// クライアントスレッドが受け取る構造体。
typedef struct client_thread{
    CLIENT* client_list;
    int standby_index;
} FOR_CT;

// クライアントが接続をやめるときの処理
void client_data_clear(CLIENT* client);
// クライアントのリストをコンソールに表示
void show_client_list(CLIENT* client_list);
// 参加中のクライアントの名前を参加中クライアントに送る。indexに名簿番号を指定で特定クライアントに送信、マイナス値を指定で参加者全員。
void send_member_list(CLIENT* client_list, int index);
// サーバからのメッセージを参加中クライアントに送る。コンソールに同じものを出力。indexに名簿番号を指定で特定クライアントに送信、マイナス値を指定で参加者全員。
void send_server_message(char* message, CLIENT* client_list, int index);

// スレッドの関数
static void *connect_accept(void *client_list);
static void *client_thread(void *for_ct);


int main(int argc, const char * argv[]) {
    
    // 入力によるコントロール用
    char buf[128];
    // スレッド
    pthread_t connect_accept_thread;
    // クライアントの名簿のようなもの。
    CLIENT client_list[CLIENT_NUM];
    
    // 名簿のゴミを消しておく
    memset(&client_list, 0, sizeof(client_list));
    
    // 接続受付用スレッドを生成
    if (pthread_create(&connect_accept_thread, NULL, connect_accept, (void *)client_list) != 0) {
        return EXIT_FAILURE;
    }
    
    // /exit を入力でループから抜ける = プログラム終了処理に進む
    while (strcmp("/exit\n", buf)) {
        // 入力
        if(fgets(buf, sizeof(buf), stdin) == NULL) {
            printf("[Message from main thread]入力エラー　終了します\n");
            break;
        }
        // /member が入力されていたら、接続中メンバーの名前を全て表示
        if(strcmp("/member\n", buf) == 0) {
            for(int i = 0; i < CLIENT_NUM ;i++) {
                if(client_list[i].client_state == joining) {
                    printf("index[%d]name[%s]\n", i, client_list[i].name);
                }
            }
        }
        // /cl が入力されていたら、名簿のnameとindexを全て表示
        if(strcmp("/cl\n", buf) == 0) {
            printf("[Message from main thread]名簿データを表示\n");
            show_client_list(client_list);
        }
    }
    
    // 接続受付用スレッドを終了、join
    pthread_cancel(connect_accept_thread);
    if (pthread_join(connect_accept_thread, NULL) != 0) {
            return EXIT_FAILURE;
        }
    // 全てのソケットをクローズ
    for(int i = 0; i < CLIENT_NUM ;i++) {
            if(client_list[i].client_state == joining) {
                close(client_list[i].sockfd);
            }
        }

    return 0;
}

// クライアントが接続をやめるときの処理
void client_data_clear(CLIENT* client) {
    close(client->sockfd);
    memset(&(client->name), 0, NAME_SIZE);
    client->client_state = empty;
}

// クライアントのリストをコンソールに表示
void show_client_list(CLIENT* client_list) {
    printf("名簿データを表示\n");
    for(int i = 0; i < CLIENT_NUM ;i++) {
        printf("index[%d] steat: %d  sockfd: %d  name[%s]\n", i, client_list[i].client_state, client_list[i].sockfd, client_list[i].name);
    }
}

// 参加中のクライアントの名前を参加中クライアントに送る。indexに名簿番号を指定で特定クライアントに送信、マイナス値を指定で参加者全員。
void send_member_list(CLIENT* client_list, int index) {
     char membars[CLIENT_NUM * NAME_SIZE] = "\e[96mServer Inco: ゲンザイ ノ サンカシャ ";

     // 名前リストを作る
     for(int i = 0; i < CLIENT_NUM ;i++) {
            if(client_list[i].client_state == joining) {
                sprintf(membars, "%s[%s] ", membars, client_list[i].name);
            }
        }
        sprintf(membars, "%s\e[0m\n", membars);

    // マイナス値を指定で参加者全員に送信。
    if(index < 0) {
        for(int i = 0; i < CLIENT_NUM ;i++) {
            if(client_list[i].client_state == joining) {
                write(client_list[i].sockfd, membars, TEXT_BUF);
            }
        }
    } else {
        write(client_list[index].sockfd, membars, TEXT_BUF);
    }
}

// サーバからのメッセージを参加中クライアントに送る。indexに名簿番号を指定で特定クライアントに送信、マイナス値を指定で参加者全員。
// これを使うことで色など統一できる。connect_acceptで名簿が埋まっていた時のメッセージだけはこの関数を使用していない点に注意。
void send_server_message(char* message, CLIENT* client_list, int index) {
    char sys_message[TEXT_BUF];

     sprintf(sys_message, "\e[96mServer Inco: %s\e[0m\n", message);


    // マイナス値を指定で参加者全員。
    if(index < 0) {
        for(int i = 0; i < CLIENT_NUM ;i++) {
            if(client_list[i].client_state == joining) {
                write(client_list[i].sockfd, sys_message, TEXT_BUF);
            }
        }
        // コンソールへ出力
        printf("%s", sys_message);
    } else {
        write(client_list[index].sockfd, sys_message, TEXT_BUF);
    }
}

// 接続してきたクライアントに名前を入力してもらい、名簿に名前を入れてsteatをjoining(参加中)に変更、そのクライアントからのメッセージの中継をする。
// このスレッドは１クライアントを１スレッドで処理する。クライアントの数だけこのスレッドを生成する。
static void *client_thread(void *for_ct) {
    FOR_CT *fct = (FOR_CT*)for_ct;
    CLIENT *cl;     // 名簿
    int index;      // このスレッドで処理するクライアントの名簿番号
    char read_buf[TEXT_BUF];
    char sys_message[TEXT_BUF]; // クライアントの名前とシステムメッセージ。
    char name_and_message[TEXT_BUF+NAME_SIZE];
    
    cl = fct->client_list;
    index = fct->standby_index;
    
    // 名前を入力してもらう。NAME_SIZEより大きいサイズなら入力しなおしてもらう
    sleep(1);
    send_server_message("コンニチハ！ Server Inco デス！", cl, index);
    sleep(2);
    send_server_message("チャット ニ サンカ シマス", cl, index);
    sleep(1);
    send_server_message("/exit ヲ ニュウリョク デ セツゾク ヲ シュウリョウ デキマス", cl, index);
    sleep(2);
    send_server_message("キミ ノ ナマエハ？", cl, index);
    // read(cl[index].sockfd, read_buf, sizeof(read_buf));
    // strtok(read_buf, "\n"); // 改行を消す
    for (;;) {
        read(cl[index].sockfd, read_buf, sizeof(read_buf));
        strtok(read_buf, "\n"); // 改行を消す
        // /exit入力ならこのスレッドを終了
        if(strcmp("\n", read_buf) == 0) {
            send_server_message("ナマエ ヲ オシエテ！", cl, index);
            continue;
        } else if(strcmp("/exit", read_buf) == 0) {
            printf("名前入力で/exitが入力されました。\n");
            client_data_clear(cl+index);
            return NULL;
        }
        else if((strlen(read_buf) < NAME_SIZE) && (strcmp(read_buf, "") != 0)) {
            printf("名前チェック通過\n");
            break;
        } else {
            send_server_message("ゴメンネ、ナガスギテ オボエラレナイヤ", cl, index);
            send_server_message("アマリ ナガクナイ ナマエ ニ シテホシイナ", cl, index);
            // read(cl[index].sockfd, read_buf, sizeof(read_buf));
            // strtok(read_buf, "\n"); // 改行を消す
        }
    }
    send_server_message("OK!", cl, index);
    sleep(1);
    // 名前を名簿に入力
    strcpy(cl[index].name, read_buf);
    // steatをjoining(参加中)に変更
    cl[index].client_state = joining;
    
    // 新規参加のメッセージをコンソールへ出力
    printf("新規参加\n");
    // 新規参加のメッセージを既参加クライアントへ送信
    sprintf(sys_message, "%s サン ガ サンカ シマシタ！", cl[index].name);
    send_server_message(sys_message, cl, -1);  
    // 現在の参加者の名前をクライアントに送信
    sleep(1);
    send_member_list(cl, -1);
    
    // show_client_list(cl);
    
    // データの中継処理を行う。/exitが入力されたら終了
    for(;;) {
        read(cl[index].sockfd, read_buf, sizeof(read_buf));
        if ((strcmp(read_buf, "")) == 0) {
            continue;
        }
        // クライアントから"/exit\n"を受け取ったら接続終了。
         if((strcmp(read_buf, "/exit\n")) == 0){
             // 接続終了を参加中のクライアントに送信
             sprintf(sys_message, "%s サン ガ セツゾク ヲ シュウリョウ シマシタ！", cl[index].name);
             send_server_message(sys_message, cl, -1);
             // 名簿を更新
            client_data_clear(cl+index); 
            // 現在の参加者の名前をクライアントに送信
            sleep(1);
            send_member_list(cl, -1);
            break;
        } else {
            //参加中のクライアントに送信
            sprintf(name_and_message, "%s%s%s", cl[index].name, ": ", read_buf);
            for(int i = 0; i < CLIENT_NUM ;i++) {
                if(cl[i].client_state == joining) {
                    write(cl[i].sockfd, name_and_message, sizeof(name_and_message));
                }
            }
            // コンソールへ出力
            printf("%s", name_and_message);
        }     
    }
    return NULL;
}

// 接続待ちをする。
static void *connect_accept(void *client_list)
{
    int sockfd;
    struct sockaddr_in server_addr;
    
    // 名簿
    CLIENT *cl = (CLIENT*)client_list;

    int tmp_client_sockfd;
    struct sockaddr_in tmp_client_addr;
    
    //　ソケット生成
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        perror("[Message from connect_accept thread]reader: socket");
        exit(1);
    }
    
    // 通信ポート・アドレスの設定。0で埋めてから使う。
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT_NUM);
    
    // ソケットにアドレスを結びつける
    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("[Message from connect_accept thread]reader: socket");
        exit(1);
    }
    
    // コネクト要求をいくつまで持つか設定する
    if(listen(sockfd, 5) < 0){
        perror("[Message from connect_accept thread]reader: listen");
        close(sockfd);
        exit(1);
    }
    
    // 接続待ち
    for (;;) {
        pthread_t guide_thread;
        socklen_t len;
        int empty_flag;
        
        printf("[Message from connect_accept thread]接続受付中\n");
        len = sizeof(tmp_client_addr);
        if((tmp_client_sockfd = accept(sockfd,(struct sockaddr *)&tmp_client_addr, &len)) < 0){
            perror("reader: accept");
            exit(1);
        }
        printf("[Message from connect_accept thread]接続を受け付けました\n");
        
        // 名簿に空きがあれば、そこに接続者情報を入れてguideスレッドを生成する。
        empty_flag = 0;
        for(int i = 0; i < CLIENT_NUM ;i++) {
            if(cl[i].client_state == empty) {
                cl[i].client_state = standby;
                cl[i].client_addr = tmp_client_addr;
                cl[i].sockfd = tmp_client_sockfd;
                empty_flag = 1;
                // スレッド生成
                FOR_CT fct;
                fct.client_list = cl;
                fct.standby_index = i;
                if (pthread_create(&guide_thread, NULL, client_thread, (void *)&fct) != 0) {
                    return NULL;
                }
                // 切り離しておく
                pthread_detach(guide_thread);
                break;
            }
        }
        if(empty_flag == 0) {
            printf("[Message from connect_accept thread]参加人数上限に達しています。接続を終了します\n");
            write(tmp_client_sockfd, "\e[96mServer Inco: メンバー ガ イッパイ デス！ マタ アトデ セツゾク シテミテネ\e[0m\n", TEXT_BUF);
            close(tmp_client_sockfd);
        }
        
    }
}
