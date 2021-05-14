#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define TEXT_BUF 512
#define PORT_NUM 8000      // ポート。クライアントと揃えておく。

// データを送る。mainから呼ぶ。
void send_data(int sockfd);
// システムからのメッセージを表示する
void print_system_message(char* message);

// データを受け取る。mainからこれのスレッドを生成する。
static void *receive_data(void *sfd);

// 起動時メッセージを表示
void print_launch_message(void);

int main(int argc, const char * argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    char ip[16];
    
    //　ソケット生成
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        
        print_system_message("エラー デス");
        perror("socket");
        exit(1);
    }
    
    // メモ。127.0.0.1 なら自分自身
    print_launch_message();
    print_system_message("コンニチハ！ System Inco デス！");
    sleep(1);
    print_system_message("サーバ ニ セツゾク シマス");
    sleep(2);
    print_system_message("セツゾクサキ ノ IPアドレス ヲ ニュウリョク シテクダサイ");
    sleep(1);
    print_system_message("例) 127.0.0.1");
    
    fgets(ip, sizeof(ip), stdin);
    
    // 構造体に接続先サーバの情報入れる。0で埋めてから使う。
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(PORT_NUM);
    
    // ソケットを接続
    if (connect(sockfd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        print_system_message("エラー デス");
        perror("connect");
        close(sockfd);
        exit(1);
    }
    print_system_message("セツゾク シマシタ");
    
    // 受信用のスレッドを作成
    if (pthread_create(&receive_thread, NULL, receive_data, (void *)&sockfd) != 0) {
        return EXIT_FAILURE;
    }
    
    // 標準入力から入力してデータを送信。exitの入力で終了。
    send_data(sockfd);
    
    // 受信用のスレッドの処理キャンセルしてjoin
    if (pthread_cancel(receive_thread) != 0) {
        print_system_message("エラー デス");
        perror("pthread_cancel");
        exit(1);
    }
    if (pthread_join(receive_thread, NULL) != 0) {
        print_system_message("エラー デス");
        perror("pthread_join");
        exit(1);
    }
    
    // ソケット閉じる
    close(sockfd);
}

// 標準入力から入力してデータを送信。/exitの入力で終了。入力が失敗した時にも終了する。
void send_data(int sockfd) {
    char write_buf[TEXT_BUF];
    
    while (strcmp("/exit\n", write_buf) != 0) {
        // 入力
        if(fgets(write_buf, sizeof(write_buf), stdin) == NULL) {
            print_system_message("ニュウリョク エラー。 セツゾク ヲ シュウリョウ シマス");
            write(sockfd, "/exit\n", sizeof(write_buf));
            
            break;
        }
        printf("\e[1A");    // カーソルを一つ上に。自分が入力したものを消すため。
        // ソケットに書き込み
        write(sockfd, write_buf, sizeof(write_buf));
    }
    print_system_message("/exit ガ　ニュウリョク サレタノデ セツゾク ヲ シュウリョウ シマス");
}

// システムメッセージを表示する
void print_system_message(char* message) {
     printf("\e[93mSystem Inco: %s\e[0m\n", message);
}

// 起動時メッセージを表示
void print_launch_message(void) {
    char *msg = "-Welcome Inco Chat System-";
    int i;
    int uslp = 60000;
    for (i = 16 ; i < 21; i++) {
        printf("\e[38;5;%dm%s\n\e[1A\e[0m", i, msg);
        usleep(uslp);
    }
    for (i = 21; i < 51; i+=6) {
        printf("\e[38;5;%dm%s\n\e[1A\e[0m", i, msg);
        usleep(uslp);
    }
    for (i = 51; i > 46; i--) {
        printf("\e[38;5;%dm%s\n\e[1A\e[0m", i, msg);
        usleep(uslp);
    }
    for (i = 46; i <= 226; i+=36) {
        printf("\e[38;5;%dm%s\n\e[1A\e[0m", i, msg);
        usleep(uslp);
    }
    sleep(3);
}

// データを受信してコンソールに表示
static void *receive_data(void *sfd) {
    char read_buf[TEXT_BUF];
    int sockfd = *(int *)sfd;
    
    while (1) {
        // 受信したデータがなければコンティニュー
        if(read(sockfd, read_buf, sizeof(read_buf)) == 0) {
            continue;
        }
        printf("%s", read_buf);
        
    }
}
