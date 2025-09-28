g = 9.81; 

L = 0.205;
r = 0.3;
J_eq = 9.31e-4;
m = 1; 

n_m = 0.69; 
n_g = 0.9;
K_t = 0.00767;
K_g = 14;
R_m = 2.6;
B_eq = 1.5e-3; 

I = (n_m * n_g * K_t * K_g^2)/R_m; 
H = (n_m * n_g * K_t * K_g)/R_m; 

path = "..\workspace_parameters.m"; 
save(path);

