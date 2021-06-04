# stride-scheduling

## 実装

### proc.h
lottery-scheduling の場合と同様に、必要な情報を構造体に追加していく。
tickets：チケット枚数
stride：プロセスのストライド
pass：プロセスのpass(呼ばれるたびにstrideを加算していく)
(以下でバッグ・確認用)
called_times:呼ばれた回数

```
struct proc {
  ・・・
  int called_times;
  int stride;
  int pass;
  int tickets;
  ・・・
}; 
```

### proc.c
実際にスケジューリングを行う部分について説明する。
概要としては、

・RUNNABLEなプロセスの中で最少のpassを選び実行する。
・途中から生成されるプロセスのpassはRUNNABLEなプロセスの中で最大のものをコピーする。
・全体的にある程度Passが大きくなったらPassがオーバーフローしないようにケアする。

といったことを実装する。

#### allocproc 関数
ここではチケット枚数を初期化し、それを使ってstrideを計算している。
適当な値 NUM=120 を使い、stride=NUM/ticket枚数とした。
passは、RUNNABLEなプロセスの中で最も大きい値にする。max_pass関数でそれを探している。
```
found:
  p->pid = allocpid();
  p->state = USED;
  p->called_times = 0;
  p->tickets = 10;
  p->stride = (int) (NUM / p->tickets);
      p->state = RUNNING;
  p->pass = max_pass();
```

#### scheduler関数
まずsearch_pass()関数で、最も小さいPassを持つRUNNABLEなプロセスを取得する。
次にそれを実行するが、このプロセスのPassが一定値（BORDER=1,000,000）を超えていたら、
pass_reset関数でRUNNABLEなプロセスのPassを ％BORDER する。

その後選んだプロセスを実行・Passを更新するが、Passを更新する前にstride を計算しなおしておく。途中でtickets が変わった場合に備える。それが終わった後に、RUNNABLEなプロセスの中で最大のPass値を記録しておく。
グローバル変数 max　にそれを記録し、プロセスが途中から参加してきた場合に、maxの値をコピーすることができる。 

```
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
   
    max = 0;
    p = search_pass();
   
      // Switch to chosen process.  It is the process's job
      // to release its lock and then reacquire it
      // before jumping back to us.
      acquire(&p->lock);

      p->called_times += 1;//for debug
      if(p->pass > BORDER) pass_reset();
      p->pass += p->stride;

      p->state = RUNNING;
      c->proc = p;
      swtch(&c->context, &p->context);
      max = max_pass();
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      release(&p->lock);
  }
}
```

#### wakeup 関数
プロセスが sleep から復帰するときの関数
ここで前述したmax を使い、Passを更新しておく。(10行目)

```
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
	      p->pass = max;
      }
      release(&p->lock);
    }
  }
}
```
### 動作確認
lottery scheduling の場合と同様に、親プロセスと子プロセスを、チケット枚数を変えて並行実行する。
結果は、おおよそチケットの逆比の回数プロセスが呼ばれており、正しい動作をしたといえる。システムコールはlotteryのときとほぼ同じため、ここでは省略する。kernel/sysproc.c には書いてあります。

#### 結果
```
$ ./test_stride

child
my tickets=10
my pass = 408
calledtimes=34

parent
my tickets=20
my pass = 408
calledtimes=72
```

#### コード
```
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]){
	int pid;
	int start, end;
	start = uptime();
	if((pid = fork()) == 0){
		change_tickets(10);
		while(1){
			end = uptime();
			if((end-start) > 100)
				break;
		}
		printf("\nchild\nmy tickets=%d\nmy pass = %d\ncalledtimes=%d\n", return_tickets(), return_pass(), return_called_times());
		exit(1);

	} else if(pid > 0){
		change_tickets(20);
		while(1){
			end = uptime();
			if((end-start) > 100)
				break;
		}
		printf("\nparent\nmy tickets=%d\nmy pass = %d\ncalledtimes=%d\n", return_tickets(), return_pass(), return_called_times());
		exit(1);
		
	} else {
		exit(1);
	}

	wait(0);
	exit(1);
}

```
