#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end();
     });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
        { 
            // Parse the JSON request body
            nlohmann::json request_body = nlohmann::json::parse(req.body);

            // Validate the request body 
            uint32_t total_entities = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
            if (total_entities > NUM_ROWS * NUM_ROWS) {
                res.code = 400;
                res.body = "Too many entities";
                res.end();
                return;
            }

            // Clear the entity grid
            entity_grid.clear();
            entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0 }));
            
            // Create the entities
            for (uint32_t i = 0; i < request_body["plants"]; ++i) {
                // Create a new plant entity
                entity_t plant_entity;
                plant_entity.type = plant;
                plant_entity.energy = 0; // Initial energy for plants (you can set it as needed)
                plant_entity.age = 0;
                
                // Randomly position the plant on the grid
                uint32_t row, col;
                do {
                    row = std::rand() % NUM_ROWS;
                    col = std::rand() % NUM_ROWS;
                } while (entity_grid[row][col].type != empty); // Check if the cell is empty
                entity_grid[row][col] = plant_entity;
            }

            for (uint32_t i = 0; i < request_body["herbivores"]; ++i) {
                // Create a new herbivore entity
                entity_t herbivore_entity;
                herbivore_entity.type = herbivore;
                herbivore_entity.energy = MAXIMUM_ENERGY; // Initial energy for herbivores
                herbivore_entity.age = 0;
                
                // Randomly position the herbivore on the grid
                uint32_t row, col;
                do {
                    row = std::rand() % NUM_ROWS;
                    col = std::rand() % NUM_ROWS;
                } while (entity_grid[row][col].type != empty); // Check if the cell is empty
                entity_grid[row][col] = herbivore_entity;
            }

            for (uint32_t i = 0; i < request_body["carnivores"]; ++i) {
                // Create a new carnivore entity
                entity_t carnivore_entity;
                carnivore_entity.type = carnivore;
                carnivore_entity.energy = MAXIMUM_ENERGY; // Initial energy for carnivores
                carnivore_entity.age = 0;
                
                // Randomly position the carnivore on the grid
                uint32_t row, col;
                do {
                    row = std::rand() % NUM_ROWS;
                    col = std::rand() % NUM_ROWS;
                } while (entity_grid[row][col].type != empty); // Check if the cell is empty
                entity_grid[row][col] = carnivore_entity;
            }

            // Return the JSON representation of the entity grid
            nlohmann::json json_grid = entity_grid; 
            res.body = json_grid.dump();
            res.end();
        });

    // Endpoint to process HTTP GET requests for the next simulation iteration
CROW_ROUTE(app, "/next-iteration")
    .methods("GET"_method)([]()
    {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behavior of each entity
        for (uint32_t i = 0; i < NUM_ROWS; ++i) {
            for (uint32_t j = 0; j < NUM_ROWS; ++j) {
                if (entity_grid[i][j].type == plant) {
                    // Plant logic
                    // Check if the plant should grow
                    if (std::rand() / static_cast<double>(RAND_MAX) < PLANT_REPRODUCTION_PROBABILITY) {
                        // Choose a random adjacent empty cell for growth
                        std::vector<pos_t> empty_neighbors;
                        if (i > 0 && entity_grid[i - 1][j].type == empty) {
                            empty_neighbors.push_back({i - 1, j});
                        }
                        if (i < NUM_ROWS - 1 && entity_grid[i + 1][j].type == empty) {
                            empty_neighbors.push_back({i + 1, j});
                        }
                        if (j > 0 && entity_grid[i][j - 1].type == empty) {
                            empty_neighbors.push_back({i, j - 1});
                        }
                        if (j < NUM_ROWS - 1 && entity_grid[i][j + 1].type == empty) {
                            empty_neighbors.push_back({i, j + 1});
                        }

                        if (!empty_neighbors.empty()) {
                            // Choose a random empty neighbor
                            pos_t new_position = empty_neighbors[std::rand() % empty_neighbors.size()];
                            // Create a new plant in the new position
                            entity_grid[new_position.i][new_position.j] = {plant, 0, 0};
                        }
                    }

                    // Check the age of the plant and decompose if necessary
                    entity_grid[i][j].age++;
                    if (entity_grid[i][j].age >= PLANT_MAXIMUM_AGE) {
                        // The plant has reached its maximum age, decompose it
                        entity_grid[i][j] = {empty, 0, 0};
                    }
                }
                else if (entity_grid[i][j].type == herbivore) {
                    // Herbivore logic
                    // Check if the herbivore should move
                    if (std::rand() / static_cast<double>(RAND_MAX) < HERBIVORE_MOVE_PROBABILITY) {
                        // Choose a random adjacent empty cell for movement
                        std::vector<pos_t> empty_neighbors;
                        if (i > 0 && entity_grid[i - 1][j].type == empty) {
                            empty_neighbors.push_back({i - 1, j});
                        }
                        if (i < NUM_ROWS - 1 && entity_grid[i + 1][j].type == empty) {
                            empty_neighbors.push_back({i + 1, j});
                        }
                        if (j > 0 && entity_grid[i][j - 1].type == empty) {
                            empty_neighbors.push_back({i, j - 1});
                        }
                        if (j < NUM_ROWS - 1 && entity_grid[i][j + 1].type == empty) {
                            empty_neighbors.push_back({i, j + 1});
                        }

                        if (!empty_neighbors.empty()) {
                            // Choose a random empty neighbor
                            pos_t new_position = empty_neighbors[std::rand() % empty_neighbors.size()];
                            // Move the herbivore to the new position
                            entity_grid[new_position.i][new_position.j] = entity_grid[i][j];
                            entity_grid[i][j] = {empty, 0, 0};
                            // Deduct energy for the movement
                            entity_grid[new_position.i][new_position.j].energy -= 5;
                        }
                    }

                    // Handle other herbivore logic (feeding, reproduction, age, death)
                    // <YOUR CODE HERE>
                }
                else if (entity_grid[i][j].type == carnivore) {
                    // Carnivore logic
                    // Check if the carnivore should move
                    if (std::rand() / static_cast<double>(RAND_MAX) < CARNIVORE_MOVE_PROBABILITY) {
                        // Choose a random adjacent empty cell for movement
                        std::vector<pos_t> empty_neighbors;
                        if (i > 0 && entity_grid[i - 1][j].type == empty) {
                            empty_neighbors.push_back({i - 1, j});
                        }
                        if (i < NUM_ROWS - 1 && entity_grid[i + 1][j].type == empty) {
                            empty_neighbors.push_back({i + 1, j});
                        }
                        if (j > 0 && entity_grid[i][j - 1].type == empty) {
                            empty_neighbors.push_back({i, j - 1});
                        }
                        if (j < NUM_ROWS - 1 && entity_grid[i][j + 1].type == empty) {
                            empty_neighbors.push_back({i, j + 1});
                        }

                        if (!empty_neighbors.empty()) {
                            // Choose a random empty neighbor
                            pos_t new_position = empty_neighbors[std::rand() % empty_neighbors.size()];
                            // Move the carnivore to the new position
                            entity_grid[new_position.i][new_position.j] = entity_grid[i][j];
                            entity_grid[i][j] = {empty, 0, 0};
                            // Deduct energy for the movement
                            entity_grid[new_position.i][new_position.j].energy -= 5;
                        }
                    }

                    // Handle other carnivore logic (feeding, reproduction, age, death)
                    // <YOUR CODE HERE>
                }
            }
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump();
    });
app.port(8080).run();

return 0;
}
