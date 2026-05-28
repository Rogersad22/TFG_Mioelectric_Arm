EMG = readmatrix("Dades_Senyal_Captura28_Lluis_Interior.txt");

%Albert: Canal 3 Interior Col 1, Canal 6 Exterior Col 2
%Roger: Canal 3 Interior Col 1, Canal 6 Exterior Col 2
%Paula: Canal 6 Interior Col 2, Canal 3 Exterior Col 1
%Anna: Canal 6 Interior Col 2, Canal 3 Exterior Col 1
%Lluís: 

EMG_signal_1_Canal_3 = EMG(:,2);
EMG_signal_2_Canal_6 = EMG(:,3);
Punts = EMG(:,1);

Fs = 3000;
T = 1/Fs;
deltaf = Fs/length(Punts);
  
t = length(Punts) * T;
eix_temps = linspace(0,t-T,length(Punts));
eix_freq = linspace(0,Fs-deltaf,length(Punts));

subplot(2, 1, 1);
plot(eix_temps,EMG_signal_1_Canal_3);
title('EMG Signal 1 Canal 3');
ylabel("Amplitud [V]");
xlabel("Temps [s]");

subplot(2, 1, 2);
plot(eix_temps,EMG_signal_2_Canal_6);
title('EMG Signal 2 Canal 6');
ylabel("Amplitud [V]");
xlabel("Temps [s]");

%Analitzem la FFT

FFT_Senyal_Canal_3 = abs(fft(EMG_signal_1_Canal_3));
figure;
plot(eix_freq, FFT_Senyal_Canal_3);
title("FFT del senyal del canal 3 (Moviment exterior).");
ylabel("Amplitud normalitzada ");
xlabel("Freqüència [Hz]");

pause;

FFT_Senyal_Canal_6 = abs(fft(EMG_signal_2_Canal_6));
figure;
plot(eix_freq, FFT_Senyal_Canal_6/ max(FFT_Senyal_Canal_6));
title("FFT del senyal del canal 5 (Moviment interior).");
ylabel("Amplitud normalitzada ");
xlabel("Freqüència [Hz]");

RMS_Senyal_pre_filtre = rms(EMG_signal_1_Canal_3);

%Filtrat del senyal Canal 3

EMG_senyal_canal3_filtrada = filter(Hd_BandPass, EMG_signal_1_Canal_3);

plot(eix_temps, EMG_senyal_canal3_filtrada);

FFT_Senyal_Canal_3_filtrada = abs(fft(EMG_senyal_canal3_filtrada));
figure;
plot(eix_freq, FFT_Senyal_Canal_3_filtrada);
title("FFT del senyal del canal 3 (Moviment exterior).");
ylabel("Amplitud normalitzada ");
xlabel("Freqüència [Hz]");

EMG_senyal_canal3_filtrada = abs(EMG_senyal_canal3_filtrada);

dades = [Punts, EMG_senyal_canal3_filtrada];

% Guardar en arxius .txt
writematrix(dades, 'Dades_Senyal_Captura28_Lluis_Interior_Filtrada.txt');

RMS_Senyal_amb_filtre_PB = rms(EMG_senyal_canal3_filtrada);

MAV_Senyal_amb_filtre_PB = mean(abs(EMG_senyal_canal3_filtrada));

