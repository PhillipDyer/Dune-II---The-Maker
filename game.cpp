#include <iostream>

#include <SFML/Graphics.hpp>
#include "game.h"
#include "houses.h"



Game::Game():
    playing(true),
    screen(),
    map(nullptr),
    shouldDeselect(false)
    //map_camera(nullptr),
    //unitRepository(nullptr)
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    screen.create(sf::VideoMode(800, 600), "Dune 2 - The Maker", sf::Style::Close, settings);
    screen.setFramerateLimit(IDEAL_FPS);
    screen.setMouseCursorVisible(false);

    if (!init()){
        std::cerr << "Failed to initialized game.";
        playing = false;
    }
}

int Game::execute() {

    sf::Clock clock;

  while(playing) {
    sf::Event event;
    actionMap.clearEvents();
    sf::Time dt = clock.restart();
    while(screen.pollEvent(event)) {
      actionMap.pushEvent(event);
    }

    updateState(dt);
    render();
  }

  return 0;
}

bool Game::init() {
  if (!terrain.loadFromFile("graphics/terrain.png")) {
    std::cout << "Failed to read graphics/terrain.png data" << std::endl;
    return false;
  }

  sf::Image temp;
  if (!temp.loadFromFile("graphics/shroud_edges.bmp")) {
    std::cout << "Failed to read graphics/shroud_edges.bmp data" << std::endl;
    return false;
  }

  temp.createMaskFromColor(sf::Color(255, 0, 255));
  shroud_edges.loadFromImage(temp);

  map.reset(new Map(terrain, shroud_edges));
  map->load("maps/4PL_Mountains.ini");

  camera.reset({0,0,800,600});
  screen.setView(camera);

  sf::Image trikeShadowImage;
  trikeShadowImage.loadFromFile("graphics/Unit_Trike_s.bmp");
  trikeShadowImage.createMaskFromColor(sf::Color(255,0,255));
  sf::Texture* trikeShadowTexture = new sf::Texture;
  trikeShadowTexture->loadFromImage(trikeShadowImage);

  sf::Image selectedImage;
  selectedImage.loadFromFile("graphics/selected.bmp");
  selectedImage.createMaskFromColor(sf::Color(255, 0, 255));
  sf::Texture* selectedTexture = new sf::Texture; //more leaks!
  selectedTexture->loadFromImage(selectedImage);


   //init two players
  int idCount = 0;
  players.emplace_back(House::Sardaukar, idCount++);
  players.emplace_back(House::Harkonnen, idCount++);

  units.emplace_back(new Unit(players[0].getTexture(), *trikeShadowTexture, *selectedTexture, 256, 256, 0, *map));
  units.emplace_back(new Unit(players[1].getTexture(), *trikeShadowTexture, *selectedTexture, 300, 300, 0, *map));

  //units.emplace_back(unitRepository->create(UNIT_FRIGATE, House::Sardaukar, 3, 3, 10, SUBCELL_CENTER, players[0]));
  //units.emplace_back(unitRepository->create(UNIT_TRIKE, House::Sardaukar, 8, 8, 3, SUBCELL_CENTER, players[0]));

  //// soldiers
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Sardaukar, 14, 14, 3, SUBCELL_CENTER, players[0]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Sardaukar, 14, 14, 3, SUBCELL_UPLEFT, players[0]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Sardaukar, 14, 14, 3, SUBCELL_UPRIGHT, players[0]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Sardaukar, 14, 14, 3, SUBCELL_DOWNLEFT, players[0]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Sardaukar, 14, 14, 3, SUBCELL_DOWNRIGHT, players[0]));

  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Harkonnen, 18, 8, 3, SUBCELL_CENTER, players[1]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Harkonnen, 18, 8, 3, SUBCELL_UPLEFT, players[1]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Harkonnen, 18, 8, 3, SUBCELL_UPRIGHT, players[1]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Harkonnen, 18, 8, 3, SUBCELL_DOWNLEFT, players[1]));
  //units.emplace_back(unitRepository->create(UNIT_SOLDIER, House::Harkonnen, 18, 8, 3, SUBCELL_DOWNRIGHT, players[1]));

  //thor Actions here

  actionMap["boxStart"] = thor::Action(sf::Mouse::Left, thor::Action::PressOnce);
  actionMap["orderMove"] = thor::Action(sf::Mouse::Left, thor::Action::PressOnce);
  actionMap["singleSelect"] = thor::Action(sf::Mouse::Left, thor::Action::PressOnce);
  actionMap["boxDrag"] = thor::Action(sf::Mouse::Left, thor::Action::Hold);
  actionMap["boxRelease"] = thor::Action(sf::Mouse::Left, thor::Action::ReleaseOnce);
  actionMap["deselectAll"] = thor::Action(sf::Mouse::Right, thor::Action::PressOnce);
  actionMap["close"] = thor::Action(sf::Event::Closed) || thor::Action(sf::Keyboard::Q, thor::Action::PressOnce);
  actionMap["cameraLeft"] = thor::Action(sf::Keyboard::Left, thor::Action::Hold);
  actionMap["cameraRight"] = thor::Action(sf::Keyboard::Right, thor::Action::Hold);
  actionMap["cameraUp"] = thor::Action(sf::Keyboard::Up, thor::Action::Hold);
  actionMap["cameraDown"] = thor::Action(sf::Keyboard::Down, thor::Action::Hold);

  typedef thor::ActionContext<std::string> actionContext;

  system.connect("close", [this](actionContext){playing = false;});

  system.connect("boxRelease", [this](actionContext){
    for (auto& unit : units){
      if (box.intersects(unit->getBounds())){
        selectUnit(*unit);
      }
    }
    box.clear();
  });

  system.connect("boxStart", [this](actionContext context){
    sf::Vector2f toSet = screen.mapPixelToCoords(mouse.getHotspot(*context.event), camera);
    box.setTopLeft(toSet);
  });

  system.connect("singleSelect", [this](actionContext context){
    sf::Vector2f toCheck = screen.mapPixelToCoords(mouse.getHotspot(*context.event), camera);
    for (auto& unit : units){
      if (unit->getBounds().contains(toCheck))
        selectUnit(*unit);
    }
  });

  system.connect("deselectAll", [this](actionContext){
    shouldDeselect = true;
    mouse.setType(Mouse::Type::Default);
    for (auto& unit : units)
      unit->unselect();
  });

  system.connect("boxDrag", [this](actionContext){
    box.setBottomRight(screen.mapPixelToCoords(sf::Mouse::getPosition(screen),camera));
  });

  const float cameraSpeed = 15.f;  

  system.connect("cameraLeft", [this, cameraSpeed](actionContext) {camera.move(-cameraSpeed, 0.f);});
  system.connect("cameraRight", [this, cameraSpeed](actionContext){camera.move(cameraSpeed, 0.f); });
  system.connect("cameraUp", [this, cameraSpeed](actionContext)   {camera.move(0.f, -cameraSpeed);});
  system.connect("cameraDown", [this, cameraSpeed](actionContext) {camera.move(0.f, cameraSpeed); });

  return true;
}

void Game::render() {
  screen.clear();

  screen.setView(camera);

  screen.draw(*map);

  for (const auto& unit : units)
      screen.draw(*unit);

  map->drawShrouded(screen, sf::RenderStates::Default);

  screen.draw(box);

  screen.draw(fpsCounter);

  screen.setView(camera);

  screen.draw(mouse);

  screen.display();
}

void Game::updateState(sf::Time dt) {
  actionMap.invokeCallbacks(system, &screen);
  if (shouldDeselect){
    shouldDeselect = false;
    system.clearConnections("orderMove");
  }

  sf::Vector2f half_of_camera = camera.getSize() / 2.f;
  sf::Vector2f topLeft = camera.getCenter() - (half_of_camera);
  sf::Vector2f downRight = camera.getCenter() + (half_of_camera);

  if (topLeft.x <= Cell::TILE_SIZE) camera.setCenter(half_of_camera.x + Cell::TILE_SIZE, camera.getCenter().y);
  if (topLeft.y <= Cell::TILE_SIZE) camera.setCenter(camera.getCenter().x, half_of_camera.y + Cell::TILE_SIZE);

  int max_width = (map->getMaxWidth() + 1) * Cell::TILE_SIZE;
  int max_height = (map->getMaxHeight() + 1) * Cell::TILE_SIZE;

  if (downRight.x >= max_width) camera.setCenter(max_width - half_of_camera.x, camera.getCenter().y);
  if (downRight.y >= max_height) camera.setCenter(camera.getCenter().x, max_height - half_of_camera.y);

  mouse.setPosition(screen.mapPixelToCoords(sf::Mouse::getPosition(screen),camera));

  for (auto& unit: units){
    unit->updateState();
  }

  fpsCounter.update(dt);
  map->prepare(screen.mapPixelToCoords(sf::Vector2i(0,0)));
}


void Game::selectUnit(Unit &unit)
{
  unit.select();
  system.connect("orderMove", [this, &unit](thor::ActionContext<std::string> context){
    unit.order_move(screen.mapPixelToCoords(mouse.getHotspot(*context.event), camera));
  });
  mouse.setType(Mouse::Type::Move); //at least one unit selected...
}
