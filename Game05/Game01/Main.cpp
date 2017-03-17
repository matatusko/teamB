#include "Main.h"

int main(int argc, char* argv[])
{
   Window window;
   Textures textures;
   window.initialize();
   srand(time(NULL));

   if (!loadMedia(&textures, &window)) {
      std::cout << "Failed loading media. Terminating program." << std::endl;
   }
   else {
      gameLoop(&textures, &window);
   }

   window.cleanup();
   return 0;
}

bool loadMedia(Textures *textures, Window *window)
{
   // Set success flag
   bool success = true; 

   // Load the background texture
   if (!(textures->worldmap.loadFromFile("images/map.png", window))) {
      std::cout << "Failed to load the map texture" << std::endl;
      success = false;
   }

   // Load the character texture
   if (!(textures->character.loadFromFile("images/character.png", window))) {
      std::cout << "Failed to load the character texture" << std::endl;
      success = false;
   }

   // Load the home texture
   if (!(textures->home.loadFromFile("images/home.png", window))) {
      std::cout << "Failed to load the home texture" << std::endl;
      success = false;
   }

   // Load the npc texture and cut it into SDL_Rects
   if (!(textures->npc.loadFromFile("images/NPC.png", window))) {
      std::cout << "Failed to load the npc texture" << std::endl;
      success = false;
   }
   cutNPCSpritesheet(textures);

   // Load the font type and set the texts to proper texture
   if (!(window->font = TTF_OpenFont("images/Consolas.ttf", 20))) {
      std::cout << "Failed to load the font" << std::endl;
      success = false;
   }
   getNPCDialog(window, textures);

   return success;
}

void gameLoop(Textures* textures, Window* window)
{
   // Game loop flag and viewport render flag
   bool gameRunning = true;
   bool interactionFound = false;
   
   // Initialize main character
   Character character;
   int playerPositionX, playerPositionY;

   // Create the camera rectangle at position 0, 0 with camera's features
   SDL_Rect camera = { 0, 0, CAMERA_WIDTH, CAMERA_HEIGHT };

   // Create the dialog window
   SDL_Rect dialogViewport = { 64, 64, 600, 150 };
   int dialogNumber;

   // Initialize Event handler
   SDL_Event e;

   // Initialize the frames for rendering animations
   int currentAnimation = 0;

   // Initialize the obstacles and their locatation
   std::vector<Obstacles> obstacles;
   setObstacles(&obstacles);

   // Initialie the NPC vector
   std::vector<Npc> npcs;
   setNpc(&npcs);

   // Start the game loop
   while (gameRunning) {

      // Get the current player position
      playerPositionX = character.getPosX();
      playerPositionY = character.getPosY();

      // Select the proper animation clip to render
      currentAnimation = animateCharacter(&character);

      // Handle the main game events on queue
      while (SDL_PollEvent(&e) != 0) {
         // Check for quit signal
         if (e.type == SDL_QUIT) {
            gameRunning = false;
         }
         if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
            if (interactionFound == true) {
               interactionFound = false;
            }
            if (checkForInteraction(&character, npcs)) {
               interactionFound = true;
               dialogNumber = rand() % 5;
            }
         }
         // Handle the character events including movement and collision checks
         character.handleMovement(e);
         character.CheckForWallCollisions(textures);
         checkForObjectsCollision(&character, obstacles, npcs, playerPositionX, playerPositionY);
      }

      // Clear screen
      SDL_RenderClear(window->renderer);

      // Render the screen to the window depending on current location player is in
      if (character.getCurrentLocation() == Location::Home) {
         renderHome(window, textures, &camera);
      }
      else if (character.getCurrentLocation() == Location::World) {
         renderWorld(window, textures, &character, &camera, npcs);
      }

      if (interactionFound) {
         SDL_SetRenderDrawColor(window->renderer, 255, 255, 255, 255);
         SDL_RenderFillRect(window->renderer, &dialogViewport);
         textures->npcDialog[dialogNumber].render(window, 75, 75);
      }

      // Render character
      character.render(window, textures, camera.x, camera.y, currentAnimation);

      // Update screen
      SDL_RenderPresent(window->renderer);

      // Sleep for a short while to add pixel-styled movement
      SDL_Delay(50);
   }
}

void renderHome(Window* window, Textures* textures, SDL_Rect* camera)
{
   // Render the home in the middle of a screen and reset the camera
   camera->x = 0; camera->y = 0;
   textures->home.render(window, CAMERA_WIDTH / 2 - textures->home.getWidth() / 2,
      CAMERA_HEIGHT / 2 - textures->home.getHeight() / 2);
}

void renderWorld(Window* window, Textures* textures, Character* character, SDL_Rect* camera, std::vector<Npc> npcs)
{
   // Render background clipped to the camera screen
   textures->worldmap.render(window, 0, 0, camera);

   // Render all the NPCs
   for (auto npc : npcs) {
      npc.render(window, textures, camera->x, camera->y);
   }

   // Center the camera with the focus on the character and keep it in bounds of the map
   cameraFocus(camera, character);
}

void cameraFocus(SDL_Rect* camera, Character* character)
{
   // Center the camera over the character 
   camera->x = (character->getPosX() + TILE_SIZE / 2) - CAMERA_WIDTH / 2;
   camera->y = (character->getPosY() + TILE_SIZE / 2) - CAMERA_HEIGHT / 2;
   
   // Prevent the camera from leaving the map screen
   if (camera->x < 0) {
      camera->x = 0;
   }
   if (camera->y < 0) {
      camera->y = 0;
   }
   if (camera->x > MAP_WIDTH - camera->w) {
      camera->x = MAP_WIDTH - camera->w;
   }
   if (camera->y > MAP_HEIGHT - camera->h) {
      camera->y = MAP_HEIGHT - camera->h;
   }
}

int animateCharacter(Character* character) {

   static int frameChange = 0;
   int currentFrame = (int) character->getCurrentDirection();
   // Checks if the character is moving, and if it is increases the current movement state by +1
   // up to 4 times, when it returns back to the original when the key is pressed up
   if (character->getCharacterMoving()) {
      frameChange++;
      if (frameChange >= 8) {
         frameChange = 0;
      }
      currentFrame += frameChange / 2;
   }

   return currentFrame;
}

bool checkCollision(SDL_Rect rect1, const SDL_Rect rect2)
{
   // Find the edges of player:
   int rect1Left = rect1.x;
   int rect1Right = rect1.x + rect1.w;
   int rect1Top = rect1.y;
   int rect1Bottom = rect1.y + rect1.h;

   // Find the edges of an collision object:
   int rect2Left = rect2.x;
   int rect2Right = rect2.x + rect2.w;
   int rect2Top = rect2.y;
   int rect2Bottom = rect2.y + rect2.h;

   // Does player's right side touch with collider's left side?
   if (rect1Right < rect2Left)
      return false; // no collision
   if (rect1Left > rect2Right)
      return false;
   if (rect1Top > rect2Bottom)
      return false;
   if (rect1Bottom < rect2Top)
      return false;

   return true; // collision found!!
}

void checkForObjectsCollision(Character *character, std::vector<Obstacles> obstacles, std::vector<Npc> npcs,
   int playerPositionX, int playerPositionY)
{
   SDL_Rect playerLocation = { character->getPosX() , character->getPosY(), TILE_SIZE, TILE_SIZE };

   // Check collisions with NPC all the NPCs
   for (auto npc : npcs) {
      if (checkCollision(playerLocation, npc.getLocation())) {
         character->setPlayerPosX(playerPositionX);
         character->setPlayerPosY(playerPositionY);
      }
   }
   // Run through all the obstacle rects and check for collision
   // If collision found return sprite back to the position before the movement
   for (const auto obstacle : obstacles) {
      if (checkCollision(playerLocation, obstacle.pos)) {
         character->setPlayerPosX(playerPositionX);
         character->setPlayerPosY(playerPositionY);
      }
   }
}

bool checkForInteraction(Character *character, std::vector<Npc> npcs)
{
   // I have no idea why he locations are supposed to be like this :D I suppose 2nd and 4th if-statement make sense
   // But the 1st and 3rd are trial-and-error :D
   for (auto npc : npcs) {
      if ((npc.getLocation().x - 64 == character->getPosX()) && (npc.getLocation().y == character->getPosY() + 32)
         && (character->getCurrentDirection() == Direction::Right)) {
         return true;
      }
      if ((npc.getLocation().x - 32 == character->getPosX()) && (npc.getLocation().y == character->getPosY())
         && (character->getCurrentDirection() == Direction::Up)) {
         return true;
      }
      if ((npc.getLocation().y - 64 == character->getPosY()) && (npc.getLocation().x == character->getPosX() + 32)
         && (character->getCurrentDirection() == Direction::Down)) {
         return true;
      }
      if ((npc.getLocation().y - 32 == character->getPosY()) && (npc.getLocation().x == character->getPosX())
         && (character->getCurrentDirection() == Direction::Left)) {
         return true;
      }
   }

   return false;
}