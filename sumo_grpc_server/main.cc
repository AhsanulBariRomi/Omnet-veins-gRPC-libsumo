#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "sumo_cosimulation.grpc.pb.h"

// Include libsumo to access SUMO's memory directly
#include <libsumo/Simulation.h>
#include <libsumo/Vehicle.h>
#include <libsumo/TrafficLight.h>
#include <libsumo/Polygon.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using veinsthesis::SumoCosimulation;
using veinsthesis::StepRequest;
using veinsthesis::StepResponse;
using veinsthesis::VehicleState;

using veinsthesis::BoundaryRequest;
using veinsthesis::BoundaryResponse;

// The actual server logic
class SumoServiceImpl final : public SumoCosimulation::Service {

    Status GetNetworkBoundaries(ServerContext* context, const BoundaryRequest* request, BoundaryResponse* response) override {
        try {
            // 1. Ask libsumo for the actual map boundary box
            // This returns a TraCIPositionVector containing the bottom-left and top-right coordinates
            auto boundary = libsumo::Simulation::getNetBoundary();
            
            // 2. Extract the bottom-left corner (index 0) as our precise offsets
            response->set_offset_x(boundary.value[0].x);
            response->set_offset_y(boundary.value[0].y);
            
            std::cout << "Heads up ===> Sent dynamic Map Boundaries to OMNeT++: X=" << boundary.value[0].x << ", Y=" << boundary.value[0].y << std::endl;
            std::cout << "\n";
            return Status::OK;
            
        } catch (const std::exception& e) {
            std::cerr << "Error ====> Error fetching boundaries: " << e.what() << std::endl;
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }

    // This is the overrriden gRPC method that will be called by the client (our TraCIScenarioManager in Veins)
    Status ExecuteStep(ServerContext* context, const StepRequest* request, StepResponse* response) override {

        //std::cout << "Step Requested for target time: " << request->target_time() << "s" << std::endl;
        
        // 1. STEP THE PHYSICS (Direct memory call, no TCP!)
        libsumo::Simulation::step(request->target_time());

        // 2. GET ALL VEHICLES
        std::vector<std::string> vehicleIds = libsumo::Vehicle::getIDList();

        std::cout << "Step " << request->target_time() << "s || Cars in SUMO: " << vehicleIds.size() << std::endl;

        // 3. PACK THE BOX
        for (const std::string& vId : vehicleIds) {
            // list of the response's repeated VehicleState message field, we will add a new entry for each vehicle
            VehicleState* state = response->add_vehicles(); // Creates a new slot in the repeated list
            
            state->set_vehicle_id(vId);
            
            // Get position directly from libsumo memory
            // libsumo::TraCIPosition pos = libsumo::Vehicle::getPosition(vId);
            // state->set_position_x(pos.x);
            // state->set_position_y(pos.y);

            // Get 3D position directly from libsumo memory
            libsumo::TraCIPosition pos3d = libsumo::Vehicle::getPosition3D(vId);
            state->set_position_x(pos3d.x);
            state->set_position_y(pos3d.y);
            state->set_position_z(pos3d.z);
            
            state->set_vehicle_type(libsumo::Vehicle::getTypeID(vId));
            
            state->set_speed(libsumo::Vehicle::getSpeed(vId));
            state->set_acceleration(libsumo::Vehicle::getAcceleration(vId)); 
            state->set_angle(libsumo::Vehicle::getAngle(vId));
            state->set_road_id(libsumo::Vehicle::getRoadID(vId));
            state->set_length(libsumo::Vehicle::getLength(vId));
            state->set_width(libsumo::Vehicle::getWidth(vId));

            // LEGACY NOTE: In the old architecture, OMNeT++ had to send separate 
            // slow TCP requests for these, or hardcode them to 0. Here, we fetch 
            // them instantly from shared memory.
            state->set_height(libsumo::Vehicle::getHeight(vId));
            state->set_signals(libsumo::Vehicle::getSignals(vId));
        }

        // 4. SHIP IT BACK
        return Status::OK;
    }

    // This gRPC method fetches the current phase (Red/Yellow/Green) of every traffic light
    Status GetTrafficLights(ServerContext* context, const veinsthesis::TrafficLightRequest* request, veinsthesis::TrafficLightResponse* response) override {
        try {
            // 1. GET ALL TRAFFIC LIGHTS
            std::vector<std::string> tlIds = libsumo::TrafficLight::getIDList();
            
            // 2. PACK THE BOX
            for (const std::string& id : tlIds) {
                // Creates a new slot in the repeated list
                veinsthesis::TrafficLightState* tl = response->add_traffic_lights();
                
                tl->set_tl_id(id);
                // Fetch the phase string directly from libsumo memory (e.g., "GgGrrr")
                tl->set_state(libsumo::TrafficLight::getRedYellowGreenState(id));
            }
            
            // 3. SHIP IT BACK
            return Status::OK;
        } catch (const std::exception& e) {
            std::cerr << "Error ====> Error fetching traffic lights: " << e.what() << std::endl;
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }

        // This gRPC method fetches all static obstacles (Polygons) for the Veins ObstacleControl module
    Status GetPolygons(ServerContext* context, const veinsthesis::PolygonRequest* request, veinsthesis::PolygonResponse* response) override {
        try {
            // 1. GET ALL POLYGONS
            std::vector<std::string> polyIds = libsumo::Polygon::getIDList();
            
            // 2. PACK THE BOX
            for (const std::string& id : polyIds) {
                veinsthesis::PolygonState* poly = response->add_polygons();
                
                poly->set_poly_id(id);
                poly->set_type(libsumo::Polygon::getType(id));
                
                // 3. EXTRACT THE SHAPE COORDINATES
                // getShape returns a TraCIPositionVector containing every corner of the building
                libsumo::TraCIPositionVector shape = libsumo::Polygon::getShape(id);
                for (const auto& point : shape.value) {
                    veinsthesis::Point2D* p2d = poly->add_shape();
                    p2d->set_x(point.x);
                    p2d->set_y(point.y);
                }
            }
            
            // 4. SHIP IT BACK
            return Status::OK;
        } catch (const std::exception& e) {
            std::cerr << "Error ====> Error fetching polygons: " << e.what() << std::endl;
            return Status(grpc::StatusCode::INTERNAL, e.what());
        }
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