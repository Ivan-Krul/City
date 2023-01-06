#include <iostream>
#include <vector>
#include <memory>
#include <array>
#include <fstream>
#include <algorithm>

#define ROAD_COST 5
#define CROSSROAD_MUL_COST 15
#define STREET_MUL_COST 2

typedef uint32_t road_length_m;
typedef uint32_t currency;

struct Street;
struct Crossroad;

struct Building
{
	Street* p_street_to;
	road_length_m length_to_start;
	currency cost;
};

struct Road
{
	std::array<Crossroad*,2>crossroads_p;
	road_length_m length;
};

struct Street : public Road
{
	std::vector<Building> buildings;
};

struct Crossroad
{
	std::vector<Street*>streets_p;
};

struct City
{
	std::vector<Street> streets;
	std::vector<Crossroad> crossroads;
};

currency CalculateCostStreet(const Street& street)
{
	currency cost = street.length * ROAD_COST + street.buildings.size();

	for(const auto &b : street.buildings)
		cost += b.cost;

	return cost;
}

currency CalculateCostCity(const City& city)
{
	currency cost = 0;

	for(const auto &s : city.streets)
		cost += CalculateCostStreet(s) * STREET_MUL_COST;
	for(const auto &c : city.crossroads)
		cost += c.streets_p.size() * CROSSROAD_MUL_COST;

	return cost;
}

void CreateStreets(std::vector<Street>& streets, std::initializer_list<road_length_m> length_list)
{
	Street street;

	for(auto iter_list = length_list.begin(); iter_list != length_list.end(); iter_list++)
	{
		street.length = *iter_list;

		streets.push_back(street);
	}
}

void CreateBuildingsInStreet(Street& street, std::initializer_list<road_length_m> building_pos_list)
{
	auto iter_list = building_pos_list.begin();
	Building build = {0};

	for(size_t ind = 0; ind < street.buildings.size() && iter_list != building_pos_list.end(); (ind++, iter_list++))
	{
		build.p_street_to = &street;
		build.length_to_start = *iter_list % street.length;

		street.buildings.push_back(build);
	}
}

void SetCostOfBuildingsInStreet(Street& street, std::initializer_list<currency> currency_list)
{
	auto iter_list = currency_list.begin();

	for(size_t ind = 0; ind < street.buildings.size() && iter_list != currency_list.end(); (ind++, iter_list++))
		street.buildings[ind].cost = *iter_list;
}

void InitializeBuildingInStreet(Street& street, std::initializer_list<std::pair<road_length_m,currency>> list)
{
	Building b = {&street};

	for(auto& [road,cur] : list)
	{
		b.length_to_start = road % street.length;
		b.cost = cur;

		street.buildings.push_back(b);
	}
}

void MergeCrossroadWithStreet(Crossroad& crossroad, Street& street)
{
	crossroad.streets_p.push_back(&street);

	street.crossroads_p[street.crossroads_p[0] > (Crossroad*)0] = &crossroad;
}

/*
	Structure:
		// City
		City.streets.size() > s
		->
			City.streets[s].length
			City.streets[s].buildings.size() > b
			->
				City.streets[s].buildings[b].cost
				City.streets[s].buildings[b].length_to_start
			<-
		<-
		City.crossroads.size() > c
		->
			City.crossroads[c].^streets.size() > s
			->
				City.crossroads[c].^streets[s]
			<-
		<-
*/

bool LoadCity(City& city, const std::string dir)
{
	std::ifstream fs(dir, std::ios::binary | std::ios::in);

	if(!fs.is_open())
		return false;

	size_t bufnum = 0;
	Street* street = nullptr;
	Building* building = nullptr;
	Crossroad* crossroad = nullptr;

	fs.read(reinterpret_cast<char *>(&bufnum), sizeof(city.streets.size()));
	city.streets.resize(bufnum);

	for(size_t s = 0; s < city.streets.size(); s++)
	{
		street = &city.streets[s];

		fs.read(reinterpret_cast<char *>(&street->length), sizeof(street->length));
		fs.read(reinterpret_cast<char *>(&bufnum), sizeof(street->buildings.size()));
		street->buildings.resize(bufnum);

		for(size_t b = 0; b < street->buildings.size(); b++)
		{
			building = &street->buildings[b];
			
			fs.read(reinterpret_cast<char*>(&building->cost), sizeof(building->cost));
			fs.read(reinterpret_cast<char*>(&building->length_to_start), sizeof(building->length_to_start));
		}
	}
	fs.read(reinterpret_cast<char *>(&bufnum), sizeof(city.crossroads.size()));
	city.crossroads.resize(bufnum);

	for(size_t c = 0; c < city.crossroads.size(); c++)
	{
		crossroad = &city.crossroads[c];

		fs.read(reinterpret_cast<char *>(&bufnum), sizeof(crossroad->streets_p.size()));
		crossroad->streets_p.resize(bufnum);

		for(size_t s = 0; s < crossroad->streets_p.size(); s++)
		{
			fs.read(reinterpret_cast<char *>(&bufnum), sizeof(crossroad->streets_p.size()));

			crossroad->streets_p[s] = &city.streets[bufnum];
		}
	}
	fs.close();

	return true;
}

void SaveCity(const City& city, const std::string dir)
{
	std::ofstream fs(dir, std::ios::binary | std::ios::out);
	size_t bufnum = city.streets.size();

	fs.write(reinterpret_cast<const char*>(&bufnum),sizeof(city.streets.size()));

	for(size_t s = 0; s < city.streets.size(); s++)
	{
		auto& street = city.streets[s];
		bufnum = street.buildings.size();

		fs.write(reinterpret_cast<const char*>(&street.length), sizeof(street.length));
		fs.write(reinterpret_cast<const char*>(&bufnum),sizeof(street.buildings.size()));

		for(size_t b = 0; b < street.buildings.size(); b++)
		{
			auto& building = street.buildings[b];

			fs.write(reinterpret_cast<const char*>(&building.cost),sizeof(building.cost));
			fs.write(reinterpret_cast<const char*>(&building.length_to_start),sizeof(building.length_to_start));
		}
	}

	size_t street_index = 0;
	bufnum = city.crossroads.size();

	fs.write(reinterpret_cast<const char*>(&bufnum),sizeof(city.crossroads.size()));

	for(size_t c = 0; c < city.crossroads.size(); c++)
	{
		auto& crossroad = city.crossroads[c];
		bufnum = city.crossroads.size();

		fs.write(reinterpret_cast<const char*>(&bufnum),sizeof(crossroad.streets_p.size()));
		
		for(size_t s = 0; s < crossroad.streets_p.size(); s++)
		{
			street_index = SIZE_MAX;

			for(size_t st = 0; st < city.streets.size(); st++)
				if(&city.streets[st] == crossroad.streets_p[s])
				{
					street_index = st;
					break;
				}
			fs.write(reinterpret_cast<const char*>(&street_index), sizeof(street_index));
		}
	}
	fs.close();
}


/*

 ---0--- 0
        /|
	   / |
	  1  2
	 /   |
	1--3-2
*/

int main()
{
	City c;
	if(!LoadCity(c,"Test City.cty"))
		return 1;
	std::cout<<"Done!\n";
	
	// CreateStreets(c.streets, {7,5,8,12});
	// c.crossroads.resize(3);
	// InitializeBuildingInStreet(c.streets[0], {{1,34},{3,64},{5,65},{6,23}});
	// InitializeBuildingInStreet(c.streets[1], {{0,75},{4,43}});
	// InitializeBuildingInStreet(c.streets[2], {{6,43}});
	// InitializeBuildingInStreet(c.streets[3], {{0,53},{1,76},{1,23},{4,23},{5,32}});
	// MergeCrossroadWithStreet(c.crossroads[0], c.streets[0]);
	// MergeCrossroadWithStreet(c.crossroads[0], c.streets[1]);
	// MergeCrossroadWithStreet(c.crossroads[0], c.streets[2]);
	// MergeCrossroadWithStreet(c.crossroads[1], c.streets[1]);
	// MergeCrossroadWithStreet(c.crossroads[1], c.streets[3]);
	// MergeCrossroadWithStreet(c.crossroads[2], c.streets[3]);
	// MergeCrossroadWithStreet(c.crossroads[2], c.streets[2]);
	std::cout<<CalculateCostCity(c)<<'\n';
	// SaveCity(c,"Test City.cty");
}
