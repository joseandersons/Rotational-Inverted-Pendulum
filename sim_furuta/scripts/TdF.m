a = ( ((4*L)/(3*r)) * (I + B_eq) ) - m*r*L;

b = ((-4*L)/(3*r)) * (I + B_eq);

c = (g/r) * (I + B_eq);

d = (g/r) * ( (m*r^2) - J_eq );

numerator = [H, 0]; 
denominator = [a, b, d, c];

G = tf(numerator, denominator);

[num, den] = tfdata(G, 'v'); 
