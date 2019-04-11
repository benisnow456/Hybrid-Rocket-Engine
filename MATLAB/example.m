w = warning ('off','all');
close all;

%% Create a simulation object. 
% The simulation class run method takes the inputs:
%    1) An empty simulation object
%    2) Nozzle throat diameter
%    3) Nozzle exit diameter
%    4) Initial fuel port diameter
%    5) Oxidizer mass flow array
%    6) Time array
%
% All units in the simulation are in SI. 

%% Define simulation parameters
close all
burn_time = 10;
burn_delay = 0.5;
padding = 2;
dt = 20;

D_throat = (19/64)*0.0254;       % in meters
D_exit = 0.5*0.0254;             % in meters
D_port = (5/8)*0.0254;           % in meters

% These second order fitting coefficients were determined experimentally
O2_coeffic = [0.000010894432  -0.000720645427   0.023080625849];

%% generate a time array
size = (burn_time+padding*2)*dt+1;
time_array = linspace(-1*padding, burn_time+padding, size)';

%% generate an oxidizer flow array
burn_start = padding*dt;
burn_end = (padding+burn_time)*dt;
oxidizer_array = polyval(O2_coeffic, time_array+burn_delay);
oxidizer_array(1:burn_start) = 0;
oxidizer_array(burn_end+1:size) = 0;

%% run the simulation
sim = Simulation();
run(sim, D_throat, D_exit, D_port, oxidizer_array, time_array);

plot(sim.time, sim.thrust, 'k-');
ylabel('Thrust (N)');
xlabel('Time (s)');