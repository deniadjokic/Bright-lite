#include "reprocess_facility.h"



namespace reprocess {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    ReprocessFacility::ReprocessFacility(cyclus::Context* ctx)
        : cyclus::Facility(ctx) {};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    std::string ReprocessFacility::str() {
      return Facility::str();
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void ReprocessFacility::Tick() {
    //inventory check is done in this phase, inventories initialized in the beginning of simulation
    	std::cout << std::endl << std::endl << "~~tick~~" << std::endl;
    	std::cout << "Input inventory: " << input_inventory.quantity() << std::endl;
    	std::cout << "Waste inventory: " << waste_inventory.quantity() << std::endl;
    	for(int i = 0; i < out_inventory.size(); i++){
    	  std::cout << "Out inventory " << i << ": " << out_inventory[i].quantity() << std::endl;
    	}
    	
    	//check to see beginning of simulation
    	if(pi != 3.141592){
    	  pi = 3.141592;
    	  int nucid = 0; //for erroneous inputs this could be a long int 
    	  double eff;
    	  std::string line;
    	  //populate out_commods, out_eff from input, initiate each associated output_inventory
    	  std::ifstream fin(repro_input_path);
           if(!fin.good()){
             std::cout << "Error! Failed reading reprocessing plant input file." << '\n';
             std::cout << "  Given path: " << repro_input_path << '\n';     
             std::cout << "  Please correct the path in cyclus input file for repro_input_path variable." << '\n';        
           }
           //reads the input file and populates out_eff with the read values
	  while(getline(fin, line)){
	    if(line.find("BEGIN") == 0){
	      cyclus::toolkit::ResourceBuff temp_pass;
	      out_inventory.push_back(temp_pass); //initializes the corresponding inventory
	      std::map<int, double> tempmap;
	      while(getline(fin, line)){
	        std::istringstream iss(line);
	        if(line.find("END") == 0){break;}
	        iss >> nucid >> eff;
	        if(nucid < 10000000 || nucid > 2000000000){
	          std::cout << "Error reading isotope identifier in reprocess facility." << '\n';
	          std::cout << "Isotope: " << nucid << " isn't in the form 'zzaaammmm'." << '\n';
	        }
	        if(eff < 0 || eff > 1){
	          std::cout << "Error in isotope removal efficiency in reprocess facility." << '\n';
	          std::cout << "  Efficiency is set to " << eff << " for " << nucid << "." << '\n';
	        }	        
	        tempmap[nucid] = eff;
	      }
	    out_eff.push_back(tempmap);
	    }
           	    
	  }
         if(out_eff.size() == 0){
           std::cout << "Error populating removal efficiencies in reprocess facility. Efficiencies not read in cyclus." << '\n';
         }
    	}
    	
    	std::cout << "-//tick//-" << std::endl;
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    void ReprocessFacility::Tock() {
    //the reprocessing is done in this phase
    	std::cout << "~~tock~~" << std::endl;
  
    	
    	//Creates a vector Material (manifest) that has been popped out.
    	std::vector<cyclus::Material::Ptr> manifest;
         manifest = cyclus::ResCast<cyclus::Material>(input_inventory.PopQty(output_capacity));
         double mani_mass = output_capacity; //remaining mass during reprocessing
         double tot_mass = 0; //total mass of the reprocessed batch (all isotopes)
         std::cout << "Amount of inventory batches being reprocessed: " << manifest.size() << '\n';
         
         for(int o = 0; o < out_eff.size(); o++){
           
             for(int m = 0; m < manifest.size(); m++){
               cyclus::CompMap temp_comp; //stores the masses of extracted isotopes
               //cyclus::CompMap out_comp = out_eff[o].mass();
               cyclus::CompMap mani_comp = manifest[m]->comp()->mass();
               std::map<int, double>::iterator out_it;
               cyclus::CompMap::iterator mani_it;
               for(out_it = out_eff[o].begin(); out_it != out_eff[o].end(); ++out_it){
                 
                 for(mani_it = mani_comp.begin(); mani_it != mani_comp.end(); ++mani_it){

                   if(out_it->first == mani_it->first){
                     temp_comp[mani_it->first] += mani_mass * mani_it->second * out_it->second;
                     tot_mass += mani_mass * mani_it->second * out_it->second;
                   }
                 }
               }
               //Puts the extracted material in the corresponding out_inventory
               out_inventory[o].Push(cyclus::ResCast<cyclus::Resource>(manifest[m]->ExtractComp(tot_mass, cyclus::Composition::CreateFromMass(temp_comp))));
               
               mani_mass -= tot_mass; //remaining mass of the reprocessed material
               tot_mass = 0;               
             }
         }
         //adds the remaining materials (the waste) in the waste_inventory
         waste_inventory.PushAll(manifest);  	
    	
    	std::cout << "-//tock//-" << std::endl;
    }

    std::set<cyclus::RequestPortfolio<cyclus::Material>::Ptr> ReprocessFacility::GetMatlRequests() {
        std::cout << "~~GetReq~~" << std::endl;
        using cyclus::RequestPortfolio;
        using cyclus::Material;
        using cyclus::Composition;
        using cyclus::CompMap;
        using cyclus::CapacityConstraint;
        std::set<RequestPortfolio<Material>::Ptr> ports;
        cyclus::Context* ctx = context();
        CompMap cm;
        
        Material::Ptr target = Material::CreateUntracked(input_capacity, context()->GetRecipe("spentFuel"));
        
        RequestPortfolio<Material>::Ptr port(new RequestPortfolio<Material>());
        
        port->AddRequest(target, this, "spentFuel");
        
        ports.insert(port);        
        

        std::cout << "-//GetReq//-" << std::endl;
        return ports;
    }

    // MatlBids //
    std::set<cyclus::BidPortfolio<cyclus::Material>::Ptr>ReprocessFacility::GetMatlBids(
            cyclus::CommodMap<cyclus::Material>::type& commod_requests) {
        std::cout << "~~GetBid~~" << std::endl;

	  using cyclus::Bid;
	  using cyclus::BidPortfolio;
	  using cyclus::CapacityConstraint;
	  using cyclus::Material;
	  using cyclus::Request;
	  using cyclus::Converter;
	 
	  std::vector<cyclus::Material::Ptr> manifest;
	  std::set<BidPortfolio<Material>::Ptr> ports;
	   
	  
           if(input_inventory.quantity() == 0){return ports;}
	  manifest = cyclus::ResCast<Material>(input_inventory.PopN(input_inventory.count()));
           
           /*
           for(int i = 0; i < manifest.size(); i++){
             cyclus::Composition::Ptr comp;
             comp = manifest[i]->comp(); 
             cyclus::CompMap v = comp->mass();
             for (std::map<cyclus::Nuc, double>::const_iterator iter = v.begin(); iter != v.end(); iter++){
               std::cout << "Key: " << iter->first << " Value: " << iter->second << std::endl;
   		  
                  
             }
             std::cout << '\n';
           }
           */
           
	  BidPortfolio<Material>::Ptr port(new BidPortfolio<Material>());

	  std::vector<Request<Material>*>& requests = commod_requests[commod_out[0]]; 

	  std::vector<Request<Material>*>::iterator it;
	  for (it = requests.begin(); it != requests.end(); ++it) {
	    std::cout << "for loop" << std::endl;
	    Request<Material>* req = *it;
	    Material::Ptr offer = Material::CreateUntracked(output_capacity, manifest[0]->comp());
	    port->AddBid(req, offer, this);
	  }

	  ports.insert(port);
	  
	  //put stuff back in inventory
	  for(int i = 0; i < manifest.size(); i++){
	    input_inventory.Push(manifest[i]);
	  }

        std::cout << "-//GetReq//-" << std::endl;
        return ports;
    }

    void ReprocessFacility::AcceptMatlTrades(const std::vector< std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >& responses) {
        std::cout << "~~AcptTrades~~" << std::endl;
      
        
        std::vector< std::pair<cyclus::Trade<cyclus::Material>, cyclus::Material::Ptr> >::const_iterator it;
        for (it = responses.begin(); it != responses.end(); ++it) {
             input_inventory.Push(it->second);
        }
   
        std::cout << "-//AcptTrades//-" << std::endl;
    }

    void ReprocessFacility::GetMatlTrades(const std::vector< cyclus::Trade<cyclus::Material> >& trades,
                                        std::vector<std::pair<cyclus::Trade<cyclus::Material>,
                                        cyclus::Material::Ptr> >& responses) {
        std::cout << "~~GetTrades~~" << std::endl;
	  using cyclus::Material;
	  using cyclus::Trade;
/*
	  std::vector< cyclus::Trade<cyclus::Material> >::const_iterator it;
	  cyclus::Material::Ptr discharge = cyclus::ResCast<Material>(fuel_inventory.Pop());
	  for (it = trades.begin(); it != trades.end(); ++it) {
	    it->request->commodity() = 
	    responses.push_back(std::make_pair(*it, discharge));
	  }
	  */
        std::cout << "-//AcptTrades//-" << std::endl;
    }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    extern "C" cyclus::Agent* ConstructReprocessFacility(cyclus::Context* ctx) {
        return new ReprocessFacility(ctx);
    }

}
