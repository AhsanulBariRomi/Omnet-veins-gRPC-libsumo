#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "sumo_cosimulation.grpc.pb.h"

// Include libsumo to access SUMO's memory directly
#include <libsumo/Simulation.h>
#include <libsumo/Vehicle.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using veinsthesis::SumoCosimulation;
using veinsthesis::StepRequest;
using veinsthesis::StepResponse;
using veinsthesis::VehicleState;

// The actual server logic
class SumoServiceImpl final : public SumoCosimulation::Service {
    // This is the overrriden gRPC method that will be called by the client (our TraCIScenarioManager in Veins)
    Status ExecuteStep(ServerContext* context, const StepRequest* request, StepResponse* response) override {
        std::cout << "Step Requested for target time: " << request->target_time() << "s" << std::endl;
        // 1. STEP THE PHYSICS (Direct memory call, no TCP!)
        libsumo::Simulation::step(request->target_time());

        // 2. GET ALL VEHICLES
        std::vector<std::string> vehicleIds = libsumo::Vehicle::getIDList();

        // 3. PACK THE BOX
        for (const std::string& vId : vehicleIds) {
            // list of the response's repeated VehicleState message field, we will add a new entry for each vehicle
            VehicleState* state = response->add_vehicles(); // Creates a new slot in the repeated list
            
            state->set_vehicle_id(vId);
            
            // Get position directly from libsumo memory
            libsumo::TraCIPosition pos = libsumo::Vehicle::getPosition(vId);
            state->set_position_x(pos.x);
            state->set_position_y(pos.y);
            
            state->set_speed(libsumo::Vehicle::getSpeed(vId));
            state->set_angle(libsumo::Vehicle::getAngle(vId));
            state->set_road_id(libsumo::Vehicle::getRoadID(vId));
            state->set_length(libsumo::Vehicle::getLength(vId));
            state->set_width(libsumo::Vehicle::getWidth(vId));
        }

        // 4. SHIP IT BACK
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    SumoServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "gRPC SUMO Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    // Start libsumo in the background (we pass the sumo cfg file here)
    libsumo::Simulation::start({"sumo", "-c", "erlangen.sumo.cfg"});
    RunServer();
    libsumo::Simulation::close();
    return 0;
}