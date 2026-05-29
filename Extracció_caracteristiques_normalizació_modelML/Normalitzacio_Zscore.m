data = readmatrix("Feature_Extractions\Test_Train_Albert\Conjunt_no_normalitzat.txt");
%data = readmatrix("Feature_Extractions\Test_SVM_V4\Dataset_Test_No_Normalitzat.txt");

R = 126;

%300 per el test 570 per el train

%{
RMS = data(:,1);
MAV = data(:,2);
WL = data(:,3);
SSC = data(:,4);
%}
label = data(:,8);


%{

for i = 0:length(label)
    if label(i) == 0
        Num_repos = Num_repos + 1;
   
    elseif label(i) == 1
        Num_gest1 = Num_gest1 + 1;

    elseif label(i) == 2
        Num_gest2 = Num_gest2 + 1;
    
    end
    
end
%}

idxRepos = find(label == 0);
idxGest1 = find(label == 1);
idxGest2 = find(label == 2); 

n_mostres1 = R; %300 per el test 570 per el train
n_mostres2 = R; 
indicesAleatoris = randperm(length(idxRepos), n_mostres1);
idxReposReduit = idxRepos(indicesAleatoris);
indicesAleatoris2 = randperm(length(idxGest1), n_mostres2);
idxGest1Reduit2 = idxGest1(indicesAleatoris2);
nousIndex = [idxReposReduit; idxGest1Reduit2; idxGest2];
datasetEquilibrat = data(nousIndex, :);
datasetEquilibrat = datasetEquilibrat(randperm(size(datasetEquilibrat,1)), :);

RMS = datasetEquilibrat(:,1);
RMSD = datasetEquilibrat(:,2);
MAV = datasetEquilibrat(:,3);
WL = datasetEquilibrat(:,4);
WLD = datasetEquilibrat(:,5);
SSC = datasetEquilibrat(:,6);
MCR = datasetEquilibrat(:,7);
label = datasetEquilibrat(:,8);


Num_repos = sum(label == 0);
Num_gest1 = sum(label == 1);
Num_gest2 = sum(label == 2);


mu_RMS = mean(RMS);   
sigma_RMS = std(RMS);   
X_norm_RMS = (RMS- mu_RMS) ./ sigma_RMS;

mu_RMSD = mean(RMSD);   
sigma_RMSD = std(RMSD);   
X_norm_RMSD = (RMSD- mu_RMSD) ./ sigma_RMSD;

mu_MAV = mean(MAV);   
sigma_MAV = std(MAV);   
X_norm_MAV = (MAV - mu_MAV) ./ sigma_MAV;

mu_WL = mean(WL);   
sigma_WL = std(WL);   
X_norm_WL = (WL - mu_WL) ./ sigma_WL;

mu_WLD = mean(WLD);   
sigma_WLD = std(WLD);   
X_norm_WLD = (WLD - mu_WLD) ./ sigma_WLD;

mu_SSC = mean(SSC);   
sigma_SSC = std(SSC);   
X_norm_SSC = (SSC - mu_SSC) ./ sigma_SSC;

mu_MCR = mean(MCR);   
sigma_MCR = std(MCR);   
X_norm_MCR = (MCR - mu_MCR) ./ sigma_MCR;

data_normalitzada = [X_norm_RMS, X_norm_RMSD, X_norm_MAV, X_norm_WL, X_norm_WLD, X_norm_SSC, X_norm_MCR, label];


output_filename = 'Feature_Extractions\Test_Train_Albert\Conjunt_Normalitzat.txt'; 
%output_filename = 'Feature_Extractions\Test_Train_Roger\Conjunt_Normalitzat.txt';
writematrix(data_normalitzada, output_filename, 'Delimiter', ',');


fprintf('Procés finalitzat. S''ha creat el fitxer: %s\n', output_filename);
