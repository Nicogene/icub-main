/*
 * Copyright (C) 2007-2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * author:  Arjan Gijsberts
 * email:   arjan.gijsberts@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include <iostream>
#include <stdexcept>
#include <cassert>

#include <yarp/IOException.h>

#include "iCub/TrainModule.h"
#include "iCub/EventDispatcher.h"
#include "iCub/TrainEvent.h"

namespace iCub {
namespace learningmachine {

void TrainProcessor::onRead(PortablePair<Vector,Vector>& sample) {
    if(this->getMachinePortable().hasWrapped() && this->enabled) {
        try {
            // Event Code
            if(EventDispatcher::instance().hasListeners()) {
                Vector prediction = this->getMachine().predict(sample.head);
                TrainEvent te(sample.head, sample.body, prediction);
                EventDispatcher::instance().raise(te);
            }
            // Event Code

            this->getMachine().feedSample(sample.head, sample.body);

        } catch(const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    return;
}


void TrainModule::printOptions(std::string error) {
    if(error != "") {
        std::cout << "Error: " << error << std::endl;
    }
    std::cout << "Available options for training module" << std::endl;
    std::cout << "--help                 Display this help message" << std::endl;
    std::cout << "--machine type         Desired type of learning machine" << std::endl;
    std::cout << "--port pfx             Prefix for registering the ports" << std::endl;
}


void TrainModule::registerAllPorts() {
    // ports from PredictModule, without model:i
    //this->registerPort(this->model_in, "/" + this->portPrefix + "/model:i");
    this->registerPort(this->predict_inout, this->portPrefix + "/predict:io");
    this->predict_inout.setStrict();
    this->registerPort(this->cmd_in, this->portPrefix + "/cmd:i");

    this->registerPort(this->model_out, this->portPrefix + "/model:o");
    this->registerPort(this->train_in, this->portPrefix + "/train:i");
    this->train_in.setStrict();
}

void TrainModule::unregisterAllPorts() {
    PredictModule::unregisterAllPorts();
    this->train_in.close();
    this->model_out.close();
}

bool TrainModule::interruptModule() {
    PredictModule::interruptModule();
    train_in.interrupt();
    return true;
}

bool TrainModule::configure(ResourceFinder& opt) {
    /* Implementation note:
     * Calling open() in the base class (i.e. PredictModule) is cumbersome due
     * to different ordering and dynamic binding (e.g. it calls
     * registerAllPorts()) and because we do bother with an incoming model port.
     */

    // read for the general specifiers:
    Value* val;
    std::string machineName;

    // cache resource finder
    this->setResourceFinder(&opt);

    // check for help request
    if(opt.check("help")) {
        this->printOptions();
        return false;
    }

    // check for port specifier: portSuffix
    if(opt.check("port", val)) {
        this->portPrefix = val->asString().c_str();
    }

    // check machine name (mandatory)
    if(opt.check("machine", val)) {
        machineName = val->asString().c_str();
    } else {
        this->printOptions("No machine type specified");
        return false;
    }

    // construct new machine
    this->getMachinePortable().setWrapped(machineName);

    // send configuration options to the machine
    this->getMachine().configure(opt);

    // add replier for incoming data (prediction requests)
    this->predict_inout.setReplier(this->predictProcessor);

    // add processor for incoming data (training samples)
    this->train_in.useCallback(trainProcessor);

    // register ports before connecting
    this->registerAllPorts();

    // and finally load command file
    if(opt.check("commands", val)) {
        this->loadCommandFile(val->asString().c_str());
    }

    // attach to the incoming command port and terminal
    this->attach(cmd_in);
    this->attachTerminal();

    return true;
}


bool TrainModule::respond(const Bottle& cmd, Bottle& reply) {
    // NOTE: the module class spawns a new thread, which implies that exception
    // handling needs to be done in this thread, so not the 'main' thread.
    bool success = false;

    try {
        switch(cmd.get(0).asVocab()) {
            case VOCAB4('h','e','l','p'): // print help information
                reply.add(Value::makeVocab("help"));

                reply.addString("Training module configuration options");
                reply.addString("  help                  Displays this message");
                reply.addString("  train                 Trains the machine and sends the model");
                reply.addString("  model                 Sends the model to the prediction module");
                reply.addString("  reset                 Resets the machine to its current state");
                reply.addString("  info                  Outputs information about the machine");
                reply.addString("  pause                 Disable passing the samples to the machine");
                reply.addString("  continue              Enable passing the samples to the machine");
                reply.addString("  set key val           Sets a configuration option for the machine");
                reply.addString("  load fname            Loads a machine from a file");
                reply.addString("  save fname            Saves the current machine to a file");
                reply.addString("  event [cmd ...]       Sends commands to event dispatcher (see: event help)");
                reply.addString("  cmd fname             Loads commands from a file");
                reply.addString(this->getMachine().getConfigHelp().c_str());
                success = true;
                break;

            case VOCAB4('t','r','a','i'): // train the machine, implies sending model
                this->getMachine().train();
                reply.addString("Training completed.");

            case VOCAB4('m','o','d','e'): // send model
                this->model_out.write(this->machinePortable);
                reply.addString("The model has been written to the port.");
                success = true;
                break;

            case VOCAB4('c','l','e','a'): // clear machine
            case VOCAB3('c','l','r'):
            case VOCAB4('r','e','s','e'): // reset
            case VOCAB3('r','s','t'):
                this->getMachine().reset();
                reply.addString("Machine cleared.");
                success = true;
                break;

            case VOCAB4('p','a','u','s'): // pause sample stream
            case VOCAB4('d','i','s','a'): // disable
                this->trainProcessor.setEnabled(false);
                reply.addString("Sample stream to machine disabled.");
                success = true;
                break;

            case VOCAB4('c','o','n','t'): // continue sample stream
            case VOCAB4('e','n','a','b'): // enable
                this->trainProcessor.setEnabled(true);
                reply.addString("Sample stream to machine enabled.");
                success = true;
                break;

            case VOCAB4('i','n','f','o'): // information
            case VOCAB4('s','t','a','t'): // statistics
                { // prevent identifier initialization to cross borders of case
                reply.add(Value::makeVocab("help"));
                reply.addString("Machine Information: ");
                reply.addString(this->getMachine().getInfo().c_str());
                success = true;
                break;
                }

            case VOCAB4('l','o','a','d'): // load
                { // prevent identifier initialization to cross borders of case
                reply.add(Value::makeVocab("help"));
                std::string replymsg = std::string("Loading machine from '") +
                                       cmd.get(1).asString().c_str() + "'... " ;
                if(!cmd.get(1).isString()) {
                    replymsg += "failed";
                } else {
                    this->getMachinePortable().readFromFile(cmd.get(1).asString().c_str());
                    replymsg += "succeeded";
                }
                reply.addString(replymsg.c_str());
                success = true;
                break;
                }

            case VOCAB4('s','a','v','e'): // save
                { // prevent identifier initialization to cross borders of case
                reply.add(Value::makeVocab("help"));
                std::string replymsg = std::string("Saving machine to '") +
                                       cmd.get(1).asString().c_str() + "'... " ;
                if(!cmd.get(1).isString()) {
                    replymsg += "failed";
                } else {
                    this->getMachinePortable().writeToFile(cmd.get(1).asString().c_str());
                    replymsg += "succeeded";
                }
                reply.addString(replymsg.c_str());
                success = true;
                break;
                }

            case VOCAB3('s','e','t'): // set a configuration option for the machine
                { // prevent identifier initialization to cross borders of case
                Bottle property;
                /*
                 * This is a simple hack to enable multiple parameters The need
                 * for this hack lies in the fact that a group can only be found
                 * using findGroup if it is a nested list in a Bottle. If the
                 * Bottle itself is the list, then the group will _not_ be found.
                 */
                property.addList() = cmd.tail();
                std::string replymsg = "Setting configuration option ";
                bool ok = this->getMachine().configure(property);
                replymsg += ok ? "succeeded" :
                                 "failed; please check key and value type.";
                reply.addString(replymsg.c_str());
                success = true;
                break;
                }

            case VOCAB4('e','v','e','n'): // event
                { // prevent identifier initialization to cross borders of case
                success = this->dmanager.respond(cmd.tail(), reply);
                break;
                }

            case VOCAB3('c','m','d'): // cmd
            case VOCAB4('c','o','m','m'): // command
                { // prevent identifier initialization to cross borders of case
                reply.add(Value::makeVocab("help"));
                std::string replymsg;
                if(!cmd.get(1).isString()) {
                    replymsg = "Please supply a valid filename.";
                } else {
                    std::string full_fname = this->findFile(cmd.get(1).asString().c_str());
                    replymsg = std::string("Loading commands from '") +
                                           full_fname + "'... " ;
                    this->loadCommandFile(full_fname, &reply);
                    replymsg += "succeeded";
                }
                reply.addString(replymsg.c_str());
                success = true;
                break;
                }

            default:
                break;

        }
    } catch(const std::exception& e) {
        std::string msg = std::string("Error: ") + e.what();
        reply.addString(msg.c_str());
        success = true;
    }

    return success;
}

} // learningmachine
} // iCub
