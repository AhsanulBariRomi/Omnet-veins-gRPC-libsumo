//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "veins/modules/application/traci/TraCIDemo11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

#include "veins/modules/mobility/traci/TraCIScenarioManager.h"

using namespace veins;

Define_Module(veins::TraCIDemo11p);

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        accidentResolved = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
    }
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

// onWSM ==> (On Wireless Short Message)
void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // findHost()->getDisplayString().setTagArg("i", 1, "green");

    // // =========================================================
    // // ---> GRPC THESIS: TEST VEHICLE CONTROL <---
    // // =========================================================
    // veinsthesis::VehicleCommand cmd;
    
    // // Get this car's unique SUMO ID (e.g., "flow0.2")
    // std::string mySumoId = mobility->getExternalId(); 
    // cmd.set_vehicle_id(mySumoId);
    
    // // Tell the car to slam on the brakes (0 m/s)
    // cmd.set_set_speed(0.0);
    
    // // Turn on the hazard lights! (Bitmask 8 in SUMO)
    // cmd.set_set_signals(8);
    
    // // Push it to the ScenarioManager's Batch Queue!
    // veins::TraCIScenarioManagerAccess().get()->addVehicleCommand(cmd);
    // std::cout << " *** Car " << mySumoId << " received warning ==> Pushed BRAKE command to gRPC queue. ***" << std::endl;
    // =========================================================

    //if (mobility->getRoadId()[0] != ':') traciVehicle->changeRoute(wsm->getDemoData(), 9999);
    // if (!sentMessage) {
    //     sentMessage = true;
    //     // repeat the received traffic update once in 2 seconds plus some random delay
    //     wsm->setSenderAddress(myId);
    //     wsm->setSerial(3);
    //     scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
    // }
    // MOVING EVERYTHING INSIDE THE SAFETY LOCK. SO THAT CARS ONLY BRAKE IF THEY ARE IN THE DANGER ZONE (BEHIND THE CRASH)
    if (!sentMessage) {
        sentMessage = true;
        
        findHost()->getDisplayString().setTagArg("i", 1, "green");
        // 1. Push the brake command ONLY ONCE!
        veinsthesis::VehicleCommand cmd;
        std::string mySumoId = mobility->getExternalId(); 
        cmd.set_vehicle_id(mySumoId);
        cmd.set_set_speed(0.0);
        cmd.set_set_signals(8);
        cmd.set_speed_mode(0);  //Force SUMO to bypass safety checks
        veins::TraCIScenarioManagerAccess().get()->addVehicleCommand(cmd);
        
        std::cout << " *** Car " << mySumoId << " received warning ==> Pushed BRAKE command to gRPC queue. ***" << std::endl;
        // 2. Rebroadcast the warning to cars behind
        wsm->setSenderAddress(myId);
        wsm->setSerial(3);
        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        // send this message on the service channel until the counter is 3 or higher.
        // this code only runs when channel switching is enabled
        sendDown(wsm->dup());
        wsm->setSerial(wsm->getSerial() + 1);
        if (wsm->getSerial() >= 3) {
            // stop service advertisements
            stopService();
            delete (wsm);
        }
        else {
            scheduleAt(simTime() + 1, wsm);
        }
    }
    else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);

    // =========================================================
    // ---> GRPC THESIS: MANUAL ACCIDENT TRIGGER <---
    // =========================================================
    // If I am the very first car (node 0) and the time hits exactly 73 seconds...
    // 1. Read the exact parameters from the omnetpp.ini file
    int iniAccidentCount = mobility->par("accidentCount").intValue();
    double iniAccidentStart = mobility->par("accidentStart").doubleValue();
    // 2. Only crash if the .ini file actually says accidentCount > 0!
    if (findHost()->getIndex() == 0 && iniAccidentCount > 0 && simTime().dbl() >= iniAccidentStart && sentMessage == false) {
        
        findHost()->getDisplayString().setTagArg("i", 1, "red"); // Turn my icon red!
        sentMessage = true; // Make sure we only do this once
        
        // 1. Create a gRPC Command to stop myself!
        veinsthesis::VehicleCommand cmd;
        cmd.set_vehicle_id(mobility->getExternalId());
        cmd.set_set_speed(0.0); // Slam the brakes
        cmd.set_set_signals(8); // Hazard lights
        cmd.set_speed_mode(0);  // Force SUMO to bypass safety checks
        veins::TraCIScenarioManagerAccess().get()->addVehicleCommand(cmd);
        
        std::cout << "\n*** BOOM!!!! Car " << mobility->getExternalId() << " crashed at 73s, Pushed STOP to gRPC queue! ***\n" << std::endl;

        // 2. Broadcast the wireless warning message to other cars
        //2a: Car 0 creates a new Wi-Fi Packet
        TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
        populateWSM(wsm);
        wsm->setDemoData(mobility->getRoadId().c_str());
        
        // 2b. Car 0 pushes the packet down to its internal MAC/PHY antenna
        if (dataOnSch) {
            startService(Channel::sch2, 42, "Traffic Information Service");
            scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
        } else {
            sendDown(wsm);
        }
        /*
        CCH (Control Channel): 
        - The main emergency channel. Every car constantly listens to this. 
        - It is used for immediate, critical safety warnings.
        SCH (Service Channels): 
        - Secondary channels (like sch1, sch2). 
        - These are used for non-critical bulk data, like downloading map updates, traffic info, or infotainment.
        */
    }
    // =========================================================

    // =========================================================
    // ---> GRPC THESIS: .INI ACCIDENT RECOVERY <---
    // =========================================================
    double iniAccidentDuration = mobility->par("accidentDuration").doubleValue();
    
    // Only recover if: I am Car 0 AND I have crashed (sentMessage==true) AND I haven't recovered yet AND enough time has passed!
    //if (findHost()->getIndex() == 0 && sentMessage == true && accidentResolved == false && simTime().dbl() >= (iniAccidentStart + iniAccidentDuration)) {
    
    // If I have stopped (sentMessage == true) AND I haven't recovered yet AND the global clearance time is reached...
    if (sentMessage == true && accidentResolved == false && simTime().dbl() >= (iniAccidentStart + iniAccidentDuration)) {
        accidentResolved = true; // Lock it so this only runs once
        findHost()->getDisplayString().setTagArg("i", 1, "blue"); // Turn icon back to green
        
        // Push the Recovery Command
        veinsthesis::VehicleCommand cmd;
        cmd.set_vehicle_id(mobility->getExternalId());
        cmd.set_set_speed(-1.0); // -1.0 gives control back to the SUMO driver!
        cmd.set_set_signals(0);  // Turn off hazard lights
        cmd.set_speed_mode(31);  // Give the brain back to SUMO - 31 is the defult mode for SUMO vehicles
        veins::TraCIScenarioManagerAccess().get()->addVehicleCommand(cmd);
        
        std::cout << "\n*** RECOVERY! Car " << mobility->getExternalId() << " resumed driving at " << simTime().dbl() << "s! ***\n" << std::endl;
    }
    // =========================================================
}
