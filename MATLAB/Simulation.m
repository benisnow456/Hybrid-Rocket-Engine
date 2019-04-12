classdef Simulation < handle
    
    %% These properties are read only, or defined through initialization
    properties (SetAccess=private)      
        time
        mdot_O2
        mdot_fuel
        mdot_total
        fuel_burned
        OF
        thrust
        Ve
        Pc
        Pe
        temp
        Isp
    end
    
    %% These properties are hidden to simplify things
    properties (GetAccess=private)
        g0 = 9.81                  % standard gravitational acceleration
        L_fuel = 0.180975;         % length of the fuel rod, in m
        rho_fuel = 998;            % fuel density, in kg/m^3
        gamma = 1.4;               % heat capacity ratio of oxygen
        a = 0.00017;               % fuel regression rate coefficient
        n = 0.31;                  % fuel regression rate exponent
        Cstar_init = 1600;         % initial guess of Cstar, in m/s
        P_atm = 101325;            % atmospheric pressure, in Pascals 
        nozzle_angle = 30;         % angle of the diverging nozzle
        scaling = 0.7;             % scaling factor to include losses
    end
    
    methods
        function obj = Simulation()
        end
        
        %% Main simulation function
        function run(obj, D_throat, D_exit, D_port_init, mdot_O2, time)
            A_t = pi*(D_throat/2)^2;
            A_e = pi*(D_exit/2)^2;
            r_init = D_port_init/2;
            obj.mdot_O2 = mdot_O2;
            obj.time = time;
            
            assert(length(time) == length(mdot_O2),...
                'Time and oxidizer flow arrays must be the same length');
            size = length(time);
            dt = round(1/mean(diff(time)));  % steps per second
            
            %% generate empty arrays to hold combustion parameters
            r_p = zeros(size, 1);            % port radius
            A_p = zeros(size, 1);            % port area
            G_O2 = zeros(size, 1);           % oxidizer flux
            rdot = zeros(size, 1);           % fuel regression rate
            Cstar = zeros(size, 1);          % characteristic velocity
            
            obj.mdot_fuel = zeros(size, 1);  % fuel mass flow
            obj.mdot_total = zeros(size, 1); % total mass flow
            obj.OF = zeros(size, 1);         % O/F ratio
            obj.thrust = zeros(size, 1);     % thrust
            obj.Ve = zeros(size, 1);         % exhaust velocity
            obj.Pe = zeros(size, 1);         % exhaust pressure
            obj.Pc = zeros(size, 1);         % chamber pressure
            obj.temp = zeros(size, 1);       % combustion temperature
            obj.Isp = zeros(size, 1);        % Specific impulse, in s
            
            burn_start = find(mdot_O2, 1);       % find start of othe burn
            burn_end = find(mdot_O2, 1, 'last'); % find end of the burn
            Cstar(burn_start) = obj.Cstar_init;
            r_p(1:burn_start) = r_init;
            
            % create CEA object
            CEA_object = CEA;
            CEA_object.setFuel('ABS', 100, 298.15);
            CEA_object.setOxid('O2', 100, 298.15);
            CEA_object.supar = A_e/A_t;
            
            %% Loop to calculate combustion at each time step
            for i=burn_start+1:burn_end
                r_p(i) = r_p(i-1) + rdot(i-1)/dt;
                A_p(i) = pi*r_p(i)^2;
                G_O2(i) = obj.mdot_O2(i)./A_p(i);
                rdot(i) = obj.a*G_O2(i)^obj.n;
                obj.mdot_fuel(i) = rdot(i)*2*pi*r_p(i)*...
                    obj.L_fuel*obj.rho_fuel;
                obj.mdot_total(i) = obj.mdot_fuel(i)+obj.mdot_O2(i);
                obj.Pc(i) = obj.mdot_total(i)*Cstar(i-1)/A_t;
                obj.OF(i) = obj.mdot_O2(i)/obj.mdot_fuel(i);
    
                CEA_object.pressure = obj.Pc(i)/obj.P_atm; % input in atm
                CEA_object.OF = obj.OF(i);
                ioinp = CEA_object.input.rocket;
                data = CEA_object.run;
                Cstar(i) = data.CSTAR.EXIT1;
                obj.Ve(i) = data.Isp.EXIT1;
                obj.Pe(i) = data.P.EXIT1*100000; %CEA outputs P in bar
                obj.temp(i) = data.T.CHAMBER;
            end
            
            %set pressures to atmospheric if engine isn't on
            obj.Pc(1:burn_start) = obj.P_atm;
            obj.Pc(burn_end+1:size) = obj.P_atm;
            obj.Pe(1:burn_start) = obj.P_atm;
            obj.Pe(burn_end+1:size) = obj.P_atm;
            %set velocity and Isp to 0 if engine isn't on
            obj.Ve(1:burn_start) = 0;
            obj.Ve(burn_end+1:size) = 0;
            obj.Isp(1:burn_start) = 0;
            obj.Isp(burn_end+1:size) = 0;
            
            %Calculate and correct thrust
            obj.thrust = obj.mdot_total.*obj.Ve + (obj.Pe-obj.P_atm)*A_e;
            lambda = (1+cosd(obj.nozzle_angle/2))/2;
            obj.thrust = obj.thrust*lambda*obj.scaling;
            obj.Isp = obj.thrust./(obj.mdot_total*obj.g0);
            obj.fuel_burned = sum(obj.mdot_fuel)/dt;
        end        
    end
end