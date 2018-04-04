1 - BUILD ParaView avec prise en charge MPI (tester avec ParaView 5.4.1)

2 - BUILD FESAPI (avec HDF5 en static) v0.11.1.1 https://github.com/F2I-Consulting/fesapi/releases/tag/v0.11.1.1

3 - CONFIGURE avec CMAKE en renseignant les éléments suivants par les bons dossiers.

FESAPI_DIR:PATH=../install

Un FindFesapi récupèrera les éléments suivant (visible en mode advanced):
- Include path = FESAPI_INCLUDE
- Library = FESAPI_LIBRARY

ParaView_DIR:PATH=../PV5-4_Build/install/lib/cmake/paraview-5.4

Si Qt est compilé avec ParaView
Qt5_DIR:PATH=../PV5-4_Build/install/lib/cmake/Qt5
Sinon prenez votre version de Qt

4 - BUILD FESPP

5 - COPY fichier dans le dossier des librairies de ParaView
libFesapiCpp.so
libFesapiCpp.so.0.11
libFesapiCpp.so.0.11.1
libFespp.so

6 - Lancement Server (Attention bien prendre le MPI du build de ParaView)
mpirun -np 8 ./pvserver

7 - Lancement Client
./paraview

8 - Connection Client/server
File->Connect... 
renseigner les paramètres du server

9 - chargement Plugin
Tools->Manage plugins...
Load du plugin Fespp sur le server ET sur le client

10- chargement d'un fichier
	a - via epc load manager (icône)
		Cette version permet de le regroupement de fichiers et gère les objets partiels, le chargement mémoire se fait à la sélection via le Widget "Selection widget"
	b - standard File->open file
		Cette version ne permet pas de charger les objets partiels et le chargement mémoire se fait en totalité
		


