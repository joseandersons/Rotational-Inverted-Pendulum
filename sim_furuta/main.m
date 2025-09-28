run("./sim_furuta/scripts/ss_to_tf.m"); % Executa o script para obter tf a partir de espaço de estados

figure;
impulse(G_pendulum); % Plota a resposta ao impulso da tf obtida

figure;
step(G_pendulum); % Plota a respota ao degrau da tf obtida

figure;
pzmap(G_pendulum); % Plota os polos e zeros da tf obtida
