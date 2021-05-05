# What is this?

해당 과제는 과목37의 중간고사 대체과제로서, zap (http://cd.textfiles.com/hackersencyc/HACKING/UNIX/ZAP.C) 이 가지고 있는 문제점을 해결하는 과제이다. 구체적으로, zap은 wtmp, utmp 파일 내부에 EOF 가 아닌 nullbyte가 발생하도록 하고, 이는 궁극적으로 rootkit detector인 ckhwtmp로부터 들키게 된다.
나아가 lastlog는 모두 제거(nullbyte 로 채움)해버리는데, 이 또한 rootkit detector 인 chklastlog로부터 들키게 된다. 이 보고서에서는 이를 해결하기 위해 어떤 방법을 택했는지, 또 실제로 적용해 봄으로서 그 방법의 타당성 또한 증명할 것이다.
