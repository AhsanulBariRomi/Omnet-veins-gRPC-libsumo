# Co-Simulation Architecture for Veins and SUMO via gRPC

> **Master's Thesis Project**  
> **Author:** Md Ahsanul Bari ([md_ahsanul.bari@mailbox.tu-dresden.de](mailto:md_ahsanul.bari@mailbox.tu-dresden.de))  
> **Institution:** Technische Universität Dresden  
> **Chair:** Chair of Networked Systems Modeling (CMS Labs)  
> **Academic Year:** 2026  

---


## Repository Structure

```
Thesis-veins-with-gRPC/
├── grpc_proto/                     # Protocol Buffer definitions
│   └── sumo_cosimulation.proto     # gRPC service definition & command/state schemas
├── sumo_grpc_server/               # Standalone C++ gRPC Server
│   ├── CMakeLists.txt              # CMake build configuration for the server
│   ├── main.cc                     # Server implementation 
│   └── build/                      # Build directory for compiled server binary

├── veins/                          # Modified Veins Simulation Framework
│   ├── src/veins/modules/mobility/traci/
│   │   ├── TraCIScenarioManager.cc # Modified scenario manager with native gRPC client
│   │   └── TraCIScenarioManager.h
│   ├── src/makefrag                # Linker flags for gRPC and Protobuf libraries
│   └── examples/veins/             # Simulation scenarios, configurations, and results
│       ├── omnetpp.ini             # Scenario parameter sweeps & benchmark definitions
│       ├── erlangen.sumo.cfg       # SUMO scenario configuration
│       ├── erlangen.net.xml        # Road network definition
│       └── erlangen.rou.xml        # Vehicle route and demand definitions
├── LICENSE                         # GNU General Public License v2.0
└── README.md                       # Comprehensive deployment and reproduction guide
```

---

## Prerequisites & System Requirements

The framework is developed and tested on **Ubuntu Linux (20.04 / 22.04 / 24.04 LTS)** or **Windows Subsystem for Linux 2 (WSL2)**.

### 1. System Packages & Build Tools
Install C++ build essentials, CMake, bison/flex, and development headers:
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    bison \
    flex \
    libxml2-dev \
    zlib1g-dev \
    python3 \
    python3-pip \
    python3-venv
```

### 2. Protocol Buffers & gRPC C++ Libraries
Install the gRPC runtime and the protobuf compiler with the C++ gRPC plugin:
```bash
sudo apt-get install -y \
    protobuf-compiler \
    libprotobuf-dev \
    libgrpc++-dev \
    protobuf-compiler-grpc
```

### 3. SUMO & `libsumo`
Ensure SUMO and its development libraries are installed and accessible:
```bash
sudo apt-get install -y sumo sumo-tools sumo-doc
```
> **Environment Variable:** Set `SUMO_HOME` in your shell (e.g., in `~/.bashrc`):
> ```bash
> export SUMO_HOME=/usr/share/sumo   # or the path to your SUMO installation
> ```

### 4. OMNeT++ 6.2.0
OMNeT++ must be configured and built on your system:
```bash
cd /path/to/omnetpp-6.2.0
python3 -m venv .venv
source .venv/bin/activate
pip install -r python/requirements.txt
./configure
make -j$(nproc)
```

---

## Step-by-Step Installation & Build Guide

### Step 1: Clone and Set Repository Root
Open your Ubuntu/WSL terminal and navigate to the project directory:
```bash
cd /path/to/Thesis-veins-with-gRPC
export THESIS_ROOT=$(pwd)
```

### Step 2: Compile Protocol Buffer Schemas
Generate the C++ headers and source files from the `.proto` service definition, then copy them into both the C++ server and Veins client directories:

```bash
cd "$THESIS_ROOT/grpc_proto"

# Compile Protocol Buffers and gRPC C++ plugins
protoc -I=. --experimental_allow_proto3_optional \
    --cpp_out=. \
    --grpc_out=. \
    --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
    sumo_cosimulation.proto

# Distribute generated files to Server and Veins Client
cp sumo_cosimulation*.h sumo_cosimulation*.cc "$THESIS_ROOT/sumo_grpc_server/"
cp sumo_cosimulation*.h sumo_cosimulation*.cc "$THESIS_ROOT/veins/src/veins/modules/mobility/traci/"
```

### Step 3: Build the C++ SUMO gRPC Server
The standalone server binds to `libsumo` and hosts the `SumoCosimulation` service.

```bash
cd "$THESIS_ROOT/sumo_grpc_server"
mkdir -p build && cd build

cmake ..
make -j$(nproc)
```
*This produces the executable binary: `$THESIS_ROOT/sumo_grpc_server/build/server`.*


### Step 4: Configure and Compile Modified Veins
Ensure the gRPC and Protobuf libraries are linked into the Veins dynamic library (`libveins.so`).

1. **Activate OMNeT++ Environment:**
   ```bash
   cd /path/to/omnetpp-6.2.0
   source setenv
   ```

2. **Verify Linker Flags in Veins:**
   Ensure `$THESIS_ROOT/veins/src/makefrag` contains the required gRPC linker flags:
   ```makefile
   EXTRA_LIBS += -lgrpc++ -lprotobuf -lgrpc++_reflection
   LDFLAGS += -lgrpc++ -lprotobuf -lgrpc++_reflection
   ```

3. **Build Veins:**
   ```bash
   cd "$THESIS_ROOT/veins"
   ./configure
   make -j$(nproc)
   ```

---

## Running the Co-Simulation (Quick Start / Smoke Test)

The co-simulation runs in a coordinated client-server architecture using two terminal sessions.

### Terminal 1: Launch the gRPC Server
Start the standalone C++ server inside the scenario folder so it resolves relative network files:
```bash
cd "$THESIS_ROOT/veins/examples/veins"
"$THESIS_ROOT/sumo_grpc_server/build/server"
```
*Expected Server Output:*
```
Server listening on 0.0.0.0:50051
Waiting for OMNeT++ connection...
```

### Terminal 2: Launch the OMNeT++ Client
In a second terminal, initialize the OMNeT++ environment and run the simulation:
```bash
# 1. Activate OMNeT++
source /path/to/omnetpp-6.2.0/setenv

# 2. Execute Co-Simulation
cd "$THESIS_ROOT/veins/examples/veins"
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini
```

### Verification Checklist:
* **Server Terminal:** Prints initialization signals, road network boundary dimensions, and step requests up to simulation limit (`target_time = 600.0s`).
* **Client Terminal:** Displays progressive event steps (`t=...s, 100% completed`) and generates `.vec` and `.sca` result files.

---

## Reproducing Thesis Evaluations & Benchmarks

All benchmark configurations are predefined in [`veins/examples/veins/omnetpp.ini`](veins/examples/veins/omnetpp.ini). Run the gRPC server in Terminal 1 first, then execute the desired experiment from Terminal 2:

### 1. Simulation Throughput Benchmark
Measures the co-simulation execution speed across increasing time horizons (from 20s to 1600s, 5 repetitions each):
```bash
cd "$THESIS_ROOT/veins/examples/veins"

opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini \
    -c Throughput_Evaluation \
    | awk '/100% completed/ { gsub(/t=/, "", $4); gsub(/s/, "", $6); print $4 "," $6 }' \
    > grpc_throughput.csv
```

### 2. Multi-Seed Repeatability Test
Runs 5 independent simulation iterations with distinct pseudo-random number generator (PRNG) seeds:
```bash
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini \
    -c Evaluation_Seeds
```

### 3. Step Synchronization Frequency Evaluation
Evaluates synchronization latency under different stepping frequencies (0.1 Hz, 1 Hz, 10 Hz, 100 Hz):
```bash
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini \
    -c Evaluation_Frequency \
    | awk 'BEGIN { print "SEED,FREQUENCY,TIME" } /Scenario:/ { for(i=1;i<=NF;i++) { if($i ~ /\$frequency=/) { f=$i; gsub(/\$frequency=|,/, "", f); } if($i ~ /\$repetition=/) { s=$i; gsub(/\$repetition=|,/, "", s); } } } /100% completed/ { gsub(/s/, "", $6); print s "," f "," $6; fflush(); }' \
    > grpc_frequency.csv
```

### 4. End-to-End Latency & Network Overhead Profiling
To capture wire traffic for Wireshark inspection and payload comparison against legacy TraCI:
```bash
# Capture gRPC traffic on port 50051:
sudo tcpdump -i lo port 50051 -w grpc_traffic.pcap

# In another terminal, run a 20s run to analyze serialization volume:
opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini --sim-time-limit=20s
```

### 5. Execution Time Benchmark (GNU Time)
Measures pure real-world CPU and system execution time:
```bash
/usr/bin/time -p opp_run -u Cmdenv -l ../../src/veins -n .:../../src/veins omnetpp.ini --sim-time-limit=1600s
```

---

## Summary of gRPC Services & Command Structures

The gRPC contract defined in [`grpc_proto/sumo_cosimulation.proto`](grpc_proto/sumo_cosimulation.proto) defines dedicated RPC endpoints and bundles per-timestep operations into batched messages:

### 1. gRPC Service Endpoints (RPCs)
| RPC Method | Input $\rightarrow$ Output | Description |
| :--- | :--- | :--- |
| `InitializeSimulation` | `InitRequest` $\rightarrow$ `InitResponse` | Passes SUMO configuration/seed and initiates the `libsumo` session |
| `GetNetworkBoundaries` | `BoundaryRequest` $\rightarrow$ `BoundaryResponse` | Fetches map bounding box ($X_{\min}, Y_{\min}, X_{\max}, Y_{\max}$) for playground sizing |
| `GetTrafficLights` | `TrafficLightRequest` $\rightarrow$ `TrafficLightResponse` | Fetches active traffic light IDs and current lane phase states |
| `GetPolygons` | `PolygonRequest` $\rightarrow$ `PolygonResponse` | Fetches static obstacles/buildings for radio shadowing |
| `ExecuteStep` | `StepRequest` $\rightarrow$ `StepResponse` | **Core Co-Simulation Step:** Advances SUMO by $\Delta t$, applies all batched commands, and returns active vehicle states |
| `CloseSimulation` | `CloseRequest` $\rightarrow$ `CloseResponse` | Gracefully closes the simulation session and releases memory |

### 2. Batched Step Payloads (`ExecuteStep`)
Rather than sending individual commands per vehicle (which caused high IPC latency in legacy TraCI), all operations per timestep are batched inside `ExecuteStep`:

* **`StepRequest` (OMNeT++ $\rightarrow$ Server):**
  * `target_time`: The simulation timestamp OMNeT++ requests SUMO to advance to.
  * `vehicle_commands` (`repeated VehicleCommand`): Batched updates for speed, speed mode (safety bitmask), route changes, lane changes, turn signals, and vehicle removals.
  * `spawn_commands` (`repeated SpawnVehicleCommand`): Batched requests to dynamically inject new vehicles with specified routes and speeds.
* **`StepResponse` (Server $\rightarrow$ OMNeT++):**
  * `vehicles` (`repeated VehicleState`): Complete active vehicle states returned directly from SUMO memory (Position $X, Y$, Speed, Heading Angle, Road ID, Signals).

---

## License

This project is licensed under the **GNU General Public License v2.0 or later** ([GPL-2.0-or-later](LICENSE)) in accordance with the upstream licensing of Veins and SUMO.

Copyright (C) 2026 Md Ahsanul Bari  
Technische Universität Dresden  
Chair of Networked Systems Modeling (CMS Labs)
