run(".\scripts\parameters.m"); 

run(".\scripts\TdF.m");

step(G);

impulse(G);

pzmap(G) 
Grid on

%{
t = 0:0.05:40;      % vetor de tempo
u = t;              % entrada rampa
y = lsim(G,u,t);    % resposta do sistema
plot(t,y)
xlabel('Tempo (s)')
ylabel('Saída')
title('Resposta à Rampa')
%}
