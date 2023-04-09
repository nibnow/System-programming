[system programming lecture]

-project 1 phase3

myshell.h
        csapp.h csapp.c를 합침

myshell.c
        Simple shell example

makefile

조건에 맞게 구현하였으나 해결하지 못한 점이 하나 있습니다
myshell에서 myshell을 실행시 바로 정지가 됩니다.
그 이유는 SIGTTIN 때문인데 이는 job관리를 위해 fork한 child가 자신의 pid를 그룹아이디로 갖게 해서입니다.
때문에 bash에서는 이를 백그라운드 프로세스로 인식해 입력을 받으려 시도할 경우 바로 멈춥니다.
마찬가지로 터미널에서 입력을 받으려 시도하는 경우 모두 정지합니다.
백그라운드로 실행시에도 바로 정지하지만 이는 실제로도 그러하므로 맞는 것 같습니다.
고치려 며칠동안 매우 노력하였지만 끝끝내 고치지 못 하였습니다.

혹여 평가를 myshell로 하실까봐 test.c를 함께 제출합니다
이는 0부터 1씩 더하며 10초마다 한번씩 출력하는 함수입니다.
컴파일 하셔서 사용하시면 평가에 도움이 될 것 같습니다.

그외 ctrl+c,z jobs,bg,fg,kill 명령은 잘 작동하는 것을 확인했습니다.
감사합니다.