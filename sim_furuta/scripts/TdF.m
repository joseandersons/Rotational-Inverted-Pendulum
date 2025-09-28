I = (n_m * n_g * K_t * K_m * (K_g^2))/R_m; % Estava faltando o termo K_m
H = (n_m * n_g * K_t * K_g)/R_m; 

a = ( ((4*L)/(3*r)) * (I + B_eq) ) - m*r*L;

b = ((-4*L)/(3*r)) * (I + B_eq);

c = (g/r) * (I + B_eq);

d = (g/r) * ( (m*(r^2)) - J_eq );

numerator = [H, 0]; 
denominator = [a, b, d, c];

sys = tf(numerator, denominator);

[num, den] = tfdata(G, 'v'); 
