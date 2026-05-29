data_Roger = readmatrix("Feature_Extractions\Test_Train_Roger\Conjunt_Normalitzat.txt");
data_Lluis = readmatrix("Feature_Extractions\Test_Train_Lluis\Conjunt_Normalitzat_Lluis.txt");
data_Albert = readmatrix("Feature_Extractions\Test_Train_Albert\Conjunt_Normalitzat.txt");
%data = readmatrix("Feature_Extractions\Test_SVM_V4\Dataset_Test_No_Normalitzat.txt");


RMS_Roger = data_Roger(:,1);
dRMS_Roger = data_Roger(:,2);
MAV_Roger = data_Roger(:,3);
WL_Roger = data_Roger(:,4);
dWL_Roger = data_Roger(:,5);
SSC_Roger = data_Roger(:,6);
MCR_Roger = data_Roger(:,7);
label_Roger = data_Roger(:,8);

RMS_Albert = data_Albert(:,1);
dRMS_Albert = data_Albert(:,2);
MAV_Albert = data_Albert(:,3);
WL_Albert = data_Albert(:,4);
dWL_Albert = data_Albert(:,5);
SSC_Albert = data_Albert(:,6);
MCR_Albert = data_Albert(:,7);
label_Albert = data_Albert(:,8);

RMS_Lluis = data_Lluis(:,1);
dRMS_Lluis = data_Lluis(:,2);
MAV_Lluis = data_Lluis(:,3);
WL_Lluis = data_Lluis(:,4);
dWL_Lluis = data_Lluis(:,5);
SSC_Lluis = data_Lluis(:,6);
MCR_Lluis = data_Lluis(:,7);
label_Lluis = data_Lluis(:,8);

dades_comparativa = [RMS_Roger; RMS_Albert; RMS_Lluis];
grups = [ones(size(RMS_Roger)); 2*ones(size(RMS_Albert)); 3*ones(size(RMS_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del RMS entre pacients');
ylabel('Amplitud Normalitzada');

figure;
hold on; 
plot(RMS_Roger, 'r', 'DisplayName', 'Reposo (0)');
plot(RMS_Albert, 'g', 'DisplayName', 'Gesto 1');
plot(RMS_Lluis, 'b', 'DisplayName', 'Gesto 2');

title('Comparativa de RMS por Gesto');
xlabel('Muestras');
ylabel('Valor RMS');
legend show; 
grid on;
hold off;

figure;
hold on; 


plot(RMS_Roger, 'r.', 'MarkerSize', 10, 'DisplayName', 'Reposo (0)');
plot(RMS_Albert, 'g.', 'MarkerSize', 10, 'DisplayName', 'Gesto 1');
plot(RMS_Lluis, 'b.', 'MarkerSize', 10, 'DisplayName', 'Gesto 2');

title('Comparativa de RMS por Gesto (Puntos)');
xlabel('Índice de Muestra');
ylabel('Valor RMS');
legend show;
grid on;
hold off;


dades_comparativa = [dRMS_Roger; dRMS_Albert; dRMS_Lluis];
grups = [ones(size(dRMS_Roger)); 2*ones(size(dRMS_Albert)); 3*ones(size(dRMS_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del Derivada RMS entre pacients');
ylabel('Amplitud Normalitzada');

dades_comparativa = [MAV_Roger; MAV_Albert; MAV_Lluis];
grups = [ones(size(MAV_Roger)); 2*ones(size(MAV_Albert)); 3*ones(size(MAV_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del MAV entre pacients');
ylabel('Amplitud Normalitzada');

dades_comparativa = [WL_Roger; WL_Albert; WL_Lluis];
grups = [ones(size(WL_Roger)); 2*ones(size(WL_Albert)); 3*ones(size(WL_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del WL entre pacients');
ylabel('Amplitud Normalitzada');

dades_comparativa = [dWL_Roger; dWL_Albert; dWL_Lluis];
grups = [ones(size(dWL_Roger)); 2*ones(size(dWL_Albert)); 3*ones(size(dWL_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del Derivada WL entre pacients');
ylabel('Amplitud Normalitzada');

dades_comparativa = [SSC_Roger; SSC_Albert; SSC_Lluis];
grups = [ones(size(SSC_Roger)); 2*ones(size(SSC_Albert)); 3*ones(size(SSC_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del SSC entre pacients');
ylabel('Amplitud Normalitzada');

dades_comparativa = [MCR_Roger; MCR_Albert; MCR_Lluis];
grups = [ones(size(MCR_Roger)); 2*ones(size(MCR_Albert)); 3*ones(size(MCR_Lluis))];

figure;
boxplot(dades_comparativa, grups, 'Labels', {'Pacient 1', 'Pacient 3', 'Pacient 5'});
title('Comparativa de la variabilitat del MCR entre pacients');
ylabel('Amplitud Normalitzada');
