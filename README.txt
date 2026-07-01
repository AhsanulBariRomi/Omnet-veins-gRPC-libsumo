================================================================================
================================== CODE ========================================
================================================================================

📁 Thesis-veins-with-gRPC/       <-- our main VS Code workspace
│
├── 📁 grpc_proto/               <-- our .proto files
├── 📁 sumo_grpc_server/         <-- our new standalone C++ Server
│
└── 📁 veins/                    <-- THIS IS THE FULL VEINS-MASTER FOLDER
    ├── 📁 examples/             <-- (we click Run here)
    │   └── 📁 veins/
    │       └── omnetpp.ini      
    │
    └── 📁 src/                  <-- (we write our gRPC code here!)
    └── 📁 src/                  <-- (we write our gRPC code here!)
        └── 📁 veins/modules/mobility/traci/
            ├── TraCIScenarioManager.h
            └── TraCIScenarioManager.cc
			
================================================================================
================================== RUN =========================================
================================================================================
Step1. Open the WSL/Ubuntu terminal. and go to the root folder.

	>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC"
	
	
Step2. Compile the Blueprints (the proto file)

	>> cd grpc_proto
	>> protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) sumo_cosimulation.proto
	>> cp sumo_cosimulation*.h sumo_cosimulation*.cc ../sumo_grpc_server/
	>> cp sumo_cosimulation*.h sumo_cosimulation*.cc ../veins/src/veins/modules/mobility/traci/
	>> cd ..
	

Step3. Build the server 
	
	i) First we created the CMakeLists.txt
	
	for that we need the libsumo. To find the libsumo =>> /home/mdahsanulbari/sumo/src/libsumo/Simulation.h
	also any file or library having the name sumo in it =>> find /usr /home -type f \( -name "*sumo*.so*" -o -name "*sumo*.a" \) 2>/dev/null
	
	ii) Command in the WSL/Ubuntu
	
	>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/sumo_grpc_server/build"
    >> cmake ..
	>> make -j$(nproc)
	
	
Step4. Configure and Compile OMNeT++ & Veins (Linux/WSL)
   Since we added our gRPC code to TraCIScenarioManager.cc and copied the .pb.cc files into Veins, we need OMNeT++ to recompile the libveins.so framework.
   
   OMNeT++ commands (like opp_makemake, opp_run, etc.) are not permanently installed into our Linux system by default. Every time we open a fresh terminal, we have to "activate" the OMNeT++ environment first.
   
   Find your OMNeT++ Folder >> find /home /opt /usr/local /mnt/f -name "opp_makemake" 2>/dev/null
   
    4.1 Install Linux Dependencies
	   OMNeT++ requires specific C++ build tools and Python virtual environment packages.
	   
		>> sudo apt-get update
		>> sudo apt-get install bison flex libxml2-dev zlib1g-dev python3-pip python3-venv -y
		
	4.2 Setup OMNeT++ Python Environment

		>> cd "/mnt/f/Omnet/omnetpp-6.2.0"
		>> python3 -m venv .venv
		>> source .venv/bin/activate
		>> python3 -m pip install -r python/requirements.txt

	4.3 Configure OMNeT++ for Headless (CLI) Mode
		Open /mnt/f/Omnet/omnetpp-6.2.0/configure.user in VS Code and change the following flags to no to disable the heavy graphical interfaces:
		WITH_QTENV=no
		WITH_OSG=no
		WITH_OSGEARTH=no
		
	4.4 Build OMNeT++
		>> cd "/mnt/f/Omnet/omnetpp-6.2.0"
		>> ./configure
		>> make -j$(nproc)
		
	4.5 Link gRPC to the Veins Build System
		We must tell Veins to include the gRPC and Protobuf libraries during compilation.
		
		>> echo 'EXTRA_LIBS += -lgrpc++ -lprotobuf -lgrpc++_reflection' >> "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/src/makefrag"
		>> echo 'LDFLAGS += -lgrpc++ -lprotobuf -lgrpc++_reflection' >> "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/src/makefrag"
		
	4.6 Build the Modified Veins Framework
	
		First Time:
		>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins"
		>> make clean
		>> ./configure
		>> make -j$(nproc)
		
		Regular changes:
		>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins"
		make -j$(nproc)


Step5. The final test - THe dual terminal Test

	Terminal 3: The Legacy Dummy Server (Python)
		Purpose: Keeps Veins happy during the t=0 setup phase.
		
		>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins"
		 python3 sumo-launchd.py -vv -c /home/mdahsanulbari/sumo/bin/sumo

	Terminal 2: The Custom gRPC Server (C++)
		Purpose: Runs the actual SUMO simulation and feeds data via gRPC.
		
		>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/examples/veins"
		 "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/sumo_grpc_server/build/server"

	Terminal 1: The OMNeT++ Client
		Purpose: Executes the Veins simulation and requests steps from your gRPC server.
		
		# 1. Wake up OMNeT++ and the Python environment
		>> cd "/mnt/f/Omnet/omnetpp-6.2.0"
		 source .venv/bin/activate
		 source setenv
		
		# 2. Launch the simulation
		>> cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/examples/veins"
		opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini 
		
		For GUI:
		cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/examples/veins"
		./run -u Qtenv

Expected Result: Terminal 3 will print >>>> FORCING gRPC CONNECTION <<<<, and Terminal 2 will immediately begin printing the "Step Requested" logs all the way up to the simulation limit of 200s.

****************************************
./run -u Cmdenv -c WithBeaconing -r 0
****************************************





===========================================================================================================================================================
LEGACY TRACI RUN
===========================================================================================================================================================
Step 1: Start the Python Bridge (Terminal 1) - LEGACY SERVER
>> cd "/mnt/f/VEINS/veins-master"
   python3 sumo-launchd.py -vv -c sumo

Step 2: Run the Simulation (Terminal 2) - LEGACY CLIENT

# 1. Wake up the compiler tools
cd "/mnt/f/Omnet/omnetpp-6.2.0"
source setenv

# 2. Go to the example folder and run the simulation
cd examples/veins
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini

=== === === === === ===
IF WE'RE DOING ANY CHANGE IN ANY FILE:
>> cd "/mnt/f/VEINS/veins-master"
make MODE=release -j$(nproc)

IF IT DOESN'T REFLECT THE CHANGES:
Step 1: Set up the environment

Bash
source /mnt/f/Omnet/omnetpp-6.2.0/setenv
Step 2: Go to the VEINS root directory

Bash
cd /mnt/f/VEINS/veins-master
Step 3: Wipe the old cache (Do not skip this)  

Bash
make clean
Step 4: Recompile the C++ code

Bash
make MODE=release -j$(nproc)
Step 5: Run the simulation

Bash
cd examples/veins
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini

=== === === === === ===
SUMO INSTALL AND VERSION CHECK
>> sumo --version
>> sudo apt update
sudo apt install sumo sumo-tools sumo-doc



========================================================================================================================================================
========================================================================================================================================================
========================================================================================================================================================
========================================================================================================================================================
SIGNALS MEANING
0 = all off
1 = right signal blincking
2 = lef signal blincking
8 = break signal turned on

========================================================================================================================================================
========================================================================================================================================================
========================================================================================================================================================
========================================================================================================================================================
GUI VEIW (Qtenv):

1. Install the missing Qt Graphical Libraries:
	sudo apt-get update
	sudo apt-get install -y qt6-base-dev qt6-tools-dev qt6-tools-dev-tools
	
2. Recompile OMNeT++:
	cd "/mnt/f/Omnet/omnetpp-6.2.0"
	make cleanall
	./configure
	make -j$(nproc)

3. Run the GUI:
	cd "/mnt/f/4. Academic(MSc)/Thesis/Thesis-veins-with-gRPC/veins/examples/veins"
	./run -u Qtenv

Also we must make sure to make WITH_QTENV = yes in the configure.use file in (F:\Omnet\omnetpp-6.2.0)
















