EMG = readmatrix("Senyals_filtrades_i_rectificades\Lluís\Dades_Senyal_Captura23_Lluis_Exterior_Fil_Rec.txt");
% 2 INTERIOR 1 EXTERIOR %

% 1 MAV i RMS alts
% 2 MAV i RMS intermitjos
% 0 MAV i RMS baixos

%Features: RMS, MAV, WL, ZC, SSC, MCR

Z = 1; %CANVIAR PER SABER SI ES EXTERIOR O INTERIOR
R = 250; %Detecció de limit humbral

Punts = EMG(:, 1);
EMG_signal = EMG(:, 2);
EMG_signal = EMG_signal(1:47250);
N = 450;
step = N/2;
deteccion = zeros(size(EMG_signal)); 

Fs = 3000;
T = 1/Fs;
t = T * length(EMG_signal);
Eix_temps = linspace(0, t-T, length(EMG_signal));

%%plot(Eix_temps, EMG_signal);

i = 1;
Deteccions_de_Moviment = 0;
Deteccions_de_repos = 0;
derivadaRMS = 0;
derivadaWL = 0;
prev_RMS = 0;
prev_WL = 0;



function segmentSSC = calculSSC(segment, threshold)

    segmentSSC = 0;
    slope = diff(segment);
    for i = 1:length(slope)-1
        if ((slope(i) > threshold && slope(i+1) < -threshold) || ...
            (slope(i) < -threshold && slope(i+1) > threshold))
            segmentSSC = segmentSSC + 1;
        end
    end
end

function segmentMCR = calculMCR(segment, mitjana)

    segmentMCR = 0;
    for i = 1:length(segment)-1
        if ((segment(i) > mitjana && segment(i+1) < mitjana) || ...
            (segment(i) < mitjana && segment(i+1) > mitjana))
            segmentMCR = segmentMCR + 1;
        end
    end
end

while i <= (length(EMG_signal) - N + 1)

    threshold = 0.01;
    finestra = window(@hamming, N);
    segment_pur = EMG_signal(i:i+N-1);
    segment = segment_pur .* finestra;
    segmentRMS = rms(segment);
    mitjana = mean((segment));
    segmentMAV = mean(abs(segment));
    segmentWL = sum(abs(diff(segment)));
    segmentSSC = calculSSC(segment, threshold);
    segmentMCR = calculMCR(segment, mitjana);

    if i == 1
        derivadaRMS = 0;
        derivadaWL = 0;
    else
        derivadaRMS = abs(segmentRMS - prev_RMS);
        derivadaWL = abs(segmentWL - prev_WL);
    end
    
    prev_RMS = segmentRMS;
    prev_WL = segmentWL;

    % 2 INTERIOR 1 EXTERIOR %


    if R < max(segment) %LL1 115

        Deteccions_de_Moviment = Deteccions_de_Moviment + 1;
        deteccion(i:i+step-1) = 800; 
        feature_extractions = [segmentRMS, derivadaRMS, segmentMAV, segmentWL, derivadaWL, segmentSSC, segmentMCR, Z];
    elseif R > max(segment) %sLL1 115

        Deteccions_de_repos = Deteccions_de_repos + 1; 
        deteccion(i:i+step-1) = 0; 
        feature_extractions = [segmentRMS, derivadaRMS, segmentMAV, segmentWL, derivadaWL, segmentSSC, segmentMCR, 0];
    end
        
    i = i + step;
    writematrix(feature_extractions ,"Feature_Extractions/Lluís_V3/FE_Captura23_Lluis_Exterior",'WriteMode', 'append');
    

    % 2 INTERIOR 1 EXTERIOR %
end

contador_final = Deteccions_de_repos + Deteccions_de_Moviment;

figure;

plot(Eix_temps, EMG_signal, 'b');
hold on
plot(Eix_temps, deteccion, 'r');


