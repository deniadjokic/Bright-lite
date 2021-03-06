#include "fuelfab_facility.h"

namespace fuelfab {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    FuelfabFacility::FuelfabFacility(cyclus::Context* ctx)
        : cyclus::Facility(ctx) {};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    std::string FuelfabFacility::str() {
      return Facility::str();
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tick() {
        if(inventory.size() == 0){
            cyclus::toolkit::ResourceBuff resource;
            for(int i = 0; i < in_commods.size(); i++){
                inventory.push_back(resource);
            }
        }

        //outputs whats inside the facility
        std::cout << "FuelFab Inventory:" << std::endl;
        for(int i = 0; i < inventory.size(); i++){
            //sets max inventory at startup
            if(inventory[i].count() == 0){
                inventory[i].set_capacity(maximum_storage/inventory.size());
            }

            std::cout << "  " << i+1 << ": " << inventory[i].quantity() << std::endl;
            if(inventory[i].count() != 0){
                std::vector<cyclus::Material::Ptr> manifest;
                manifest = cyclus::ResCast<cyclus::Material>(inventory[i].PopN(inventory[i].count()));

                cyclus::CompMap comp;
                cyclus::CompMap::iterator it;

                comp = manifest[0]->comp()->mass();

                for(it = comp.begin(); it != comp.end(); ++it){
                    std::cout << "       " << it->first << " " << it->second << std::endl;
                }
                inventory[i].PushAll(manifest);
            }
        }
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void FuelfabFacility::Tock() {

    }

    std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> FuelfabFacility::GetMatlRequests() {
        using cyclus::RequestPortfolio;
        using cyclus::Material;
        using cyclus::Composition;
        using cyclus::CompMap;
        using cyclus::CapacityConstraint;
        std::set<RequestPortfolio<Material>::Ptr> ports;
        cyclus::Context* ctx = context();
        CompMap cm;
        Material::Ptr target = Material::CreateUntracked(maximum_storage/inventory.size(), Composition::CreateFromAtom(cm));
        RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
        for(int i = 0; i < inventory.size(); i++){
            double qty = inventory[i].space();

            port->AddRequest(target, this, in_commods[i]);

            CapacityConstraint<Material> cc(qty);
            port->AddConstraint(cc);
            ports.insert(port);
        }
        return ports;
    }

    // MatlBids //
    std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>FuelfabFacility::GetMatlBids(cyclus::CommodMap<cyclus::Material>::type& commod_requests) {
    using cyclus::BidPortfolio;
    using cyclus::CapacityConstraint;
    using cyclus::Converter;
    using cyclus::Material;
    using cyclus::Request;
    using reactor::ReactorFacility;
    cyclus::Context* ctx = context();
    std::set<BidPortfolio<Material>::Ptr> ports;

    // respond to all requests of my commodity
    int inventory_test = 0;
    for(int i = 0; i < inventory.size(); i++){
        if(inventory[i].count() > 0){
            inventory_test += 1;
        }
    }/*
    /** Quick Hack */

    CapacityConstraint<Material> cc(10000);
        if (inventory_test == 0){return ports;}

        BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());
        std::vector<Request<Material>*>& requests = commod_requests[out_commod];
        std::vector<Request<Material>*>::iterator it;
        for (it = requests.begin(); it != requests.end(); ++it) {
            Request<Material>* req = *it;
            ReactorFacility* reactor = dynamic_cast<ReactorFacility*>(req->requester());
            if (!reactor){
               throw cyclus::CastError("Nope!");
            } else {
                if (req->commodity() == out_commod) {
                    if (reactor->inventory.count() == 0){
                        reactor->start_up(inventory);
                    } /*else if(reactor->burnup_test(offer) == reactor->target_burnup){
                        port->AddBid(req, offer, this);
                        port->AddConstraint(cc);
                        ports.insert(port);
                    };*/
                }
            }
        }
    }

    void FuelfabFacility::AcceptMatlTrades(const std::vector< std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >& responses) {

        std::vector<std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >::const_iterator it;
        for (it = responses.begin(); it != responses.end(); ++it) {

            for(int i = 0; i < in_commods.size(); i++){
                if(it->first.request->commodity() == in_commods[i]){
                    inventory[i].Push(it->second);
                }

            }
        }
    }

    void FuelfabFacility::GetMatlTrades(const std::vector< cyclus::Trade<cyclus::Material> >& trades,
                                        std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                        cyclus::Material::Ptr> >& responses) {
        using cyclus::Material;
        using cyclus::Trade;
        std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;/*
        for(int i = 0; i < inventory.size(); i++){
            cyclus::Material::Ptr discharge = cyclus::ResCast<Material>(inventory[i].Pop());
            for (it = trades.begin(); it != trades.end(); ++it) {
                responses.push_back(std::make_pair(*it, discharge));
            }
        }*/
    }
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    extern "C" cyclus::Agent* ConstructFuelfabFacility(cyclus::Context* ctx) {
        return new FuelfabFacility(ctx);
    }

}
