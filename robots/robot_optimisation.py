"""
robot ecosystem operation code

This module creates a test ecosystem and runs it for a specified duration, 
demonstrating the use of the ecosystem factory and deliverable creation functions
It simulates the operation of delivery bots, including charging and delivering pizzas, while providing options for debugging and message display.
"""

from math import sqrt
from turtle import speed

from robots.ecosystem.factory import ecofactory
from robots.ecosystem.ecosystem import energy_consumption

# Duration is set to two weeks for development and rapid testing. Set to 52 weeks for your final tests.

import matplotlib.pyplot as plt
results = {}
pizzamaxweights = []
total_delivered_weights = []
total_delivered_units = []
for i in range(3):
  plt.close('all')  # optional: cleans up leftovers from prior runs
  plt.ion()         # interactive mode ON (non-blocking windows)
  # Create and configure the ecosystem using the factory function. 
  # Study the factory function code to understand how the ecosystem is being created 
  # and configured. Adjust the parameters as needed for your testing and development.  

  NumRobots = 3
  NumDroids = 3
  NumDrones = 3
  es = ecofactory(robots = NumRobots, droids = NumDroids, drones = NumDrones, chargers = [[20,20],[60,20]], pizzas = 9)
  es._max_weight = 120
  pizzamaxweights.append(es._max_weight)
  OptimalLoad = 0
  robotMaxPayloads = [22.5,80,125]
  robotkindToPayloadIndex = {'Robot': 2, 'Droid': 1, 'Drone': 0}
  es.display(show = 0, pause = 10)                                                # show = 0 will turn off the display and speed up the run. Set to 1 for development and debugging, set to 0 for final runs. Note that when show = 0, you will not see the ecosystem or any messages, so it is wise to turn on messages (es.messages_on = True) when show = 0 for development and debugging. 
  es.debug = True                                                                # this will directly display damage and warning messages. Note show needs to be zero  (show = 0)
  es.messages_on = True                                                          # over 52 weeks it is wise to turn messages off as there are too many. But when researching turn on for shorter runs
  es.duration = "52 week"                                                          # We are aiming to run for a year with minimum or no bot breakages
  pizzaAgeThreshold = 100                                                         #how long after a pizza is made it should be prioritised for delivery
  home = [40,20, 0]                                                               # Place to which bots will return when idle and from which they will start. This is also the location of the charger in this example, but it doesn't have to be. You can change this and the charger location to test the bots' ability to navigate around the ecosystem.
  charge_threshold = 0.20                                                         # this is the soc percentage at which bots will decide to charge. This can be optimised and varied for each kind (see stretch objective)                               

  def PizzaSearch(OptimalLoad, bot): #method to search through all pizzas to choose which should be delivered next by this bot
    minPizzaDist = float('inf')
    closestPizza = None
    lowWeightPizzas = 0
    medWeightPizzas = 0
    for pizza in es.deliverables(): 
       #PIZZA ALLOCATION
     if pizza.status == 'ready':
         
        #VARIABLE MAX PIZZA WEIGHT - if there are no low weight pizzas, lower the max weight so the weak bots have something to deliver
        if pizza.weight<robotMaxPayloads[0]:
          lowWeightPizzas +=1
        elif pizza.weight<robotMaxPayloads[1]:
          medWeightPizzas +=1
        #PIZZA AGE PRIORITISATION
        if pizza.age > pizzaAgeThreshold and pizza.weight < bot.max_payload:## however if a pizza is over a threshold age we will prioritise it so that pizzas arent left too long after cooking
          ##this avoids pizzas that are a long distance away from being under-prioritised
          return pizza
        #CLOSEST PIZZA
        distance = sqrt((pizza.coordinates[0] - bot.coordinates[0]) ** 2 + (pizza.coordinates[1] - bot.coordinates[1]) ** 2)
        if distance < minPizzaDist:##find closest ready pizza and assign it to our bot.
          #we will only allocate a pizza to a robot if a "weaker" robot cannot do the job
          #this means that stronger robots are carrying closer to optimal capacity and weaker robots dont lose deliveries (which are already lowered because there are less low weight deliveries when we increased the max weight of pizza)
          ##OPTIMAL LOAD
          if OptimalLoad == 1:
            if (robotkindToPayloadIndex[bot.kind] > 0):
              if pizza.weight > robotMaxPayloads[robotkindToPayloadIndex[bot.kind]-1] and pizza.weight < bot.max_payload:#if pizza is less than the max payload of this bot but more than the max payload of the next weakest bot
                minPizzaDist = distance
                closestPizza = pizza
            elif pizza.weight < bot.max_payload:##if its a droid just check if it can carry the weight
              minPizzaDist = distance
              closestPizza = pizza
          elif pizza.weight < bot.max_payload:
          ##if this is a second pass and we dont care about optimal load, just check if it can carry the weight
            minPizzaDist = distance
            closestPizza = pizza
    # #VARIABLE MAX WEIGHT - makes sure there is always atleast 1 pizza of a weight range per bot so bots arent idled
      # increasing the max weight increases our weight delivered, but it also decreases the amount of orders weaker bots can deliver. 
    if lowWeightPizzas <= NumDrones:
      es._max_weight = 22
    elif medWeightPizzas <= NumDroids:
      es._max_weight = 80
    else:
      es._max_weight = 120
    return closestPizza


  while es.active:
    for bot in es.bots():
      ##PIZZA ALLOCATION 
      #create_deliverables(es)                                                     # Use the create deliverables function to maintain a stock of ready pizzas
      if bot.activity == 'idle':                                                  # if bot is idle, contract to deliver a ready pizza.
        closestPizza = PizzaSearch(1, bot) #first search for an available pizza with an optimal load for this bot
        if closestPizza != None:
          bot.deliver(closestPizza)
        else :#if our initial search of pizzas, using the "optimal load" returned nothing, then we will search again without this parameter to make sure that no bots are left idle, because an un-optimal delivery is better than none. 
          closestPizza = PizzaSearch(0, bot)
          if closestPizza != None:
            bot.deliver(closestPizza)
      
      if not bot.destination and bot.coordinates != home:
        bot.target_destination = home                                           # if we get here, we've gone through the list of pizzas and none was ready
      if bot.target_destination:bot.move()                                        # move whilst we have a destination. At the end of delivery, the bot status will be set to idle  
      # ## CHARGER AVAILABILITY - choose the closest available charger when we decide to charge.
      if bot.station is None:       
         ##find closest available charger
        chargers = es.chargers()
        minDist = float('inf') ##set infinity so that any valid number can be set as first minumum
        closest_charger = None
        opportune_distance = 5
        charged_now = False
        for charger in chargers:
          currentChargerDist = sqrt((charger.coordinates[0] - bot.coordinates[0]) ** 2 + (charger.coordinates[1] - bot.coordinates[1]) ** 2)
          if currentChargerDist < opportune_distance and charger.status == 'vacant' and bot.soc / bot.max_soc < 0.9: ##OPPORTUNE CHARGING - if we are close to a charger, charge there no matter battery level and skip loop
            bot.charge(charger)  
            charged_now = True
            break
          if currentChargerDist < minDist: ##find closest charger 
            minDist = currentChargerDist
            closest_charger = charger
        if not charged_now and closest_charger is not None:
          #DYNAMIC CHARGE THRESHOLD - finds the energy needed to reach a charger so can set a charge threshold so that the bot can always reach it
          cargo_weight = sum(item.weight for item in getattr(bot, 'cargo', [])) #need cargo weight for energy calculation
          travel_speed = bot.max_speed if bot.max_speed > 0 else 1 # avoid 0 division error
          required_energy = energy_consumption(bot.weight + cargo_weight, travel_speed, bot.volitant) * (minDist / travel_speed)
          charge_threshold = (required_energy + (0.1 * bot.max_soc)) / bot.max_soc #has a safety of 0.1 to make sure bot doesnt get too close to 0 soc
          if bot.soc / bot.max_soc < charge_threshold: ##if still not charged (opportune charging didnt occur) and there is an available charger, charge at the closest
            bot.charge(closest_charger)        # initiate charging.
                        
    es.update() # update when all bots have been processed and moved
  results[i] = es
  print("\n*Example of tabulated results for one run:*")
  es.tabulate(
    'name', 'kind', 'status','active', 'weight_delivered', 'units_delivered', 'distance', 'energy',
    kind_class='Bot')  
  


total_weights = [
    sum(r['weight_delivered'] for r in es.registry(kind_class='Bot').values())
    for es in results.values()
]
average_weight = sum(total_weights) / len(total_weights)
total_units = [
    sum(r['units_delivered'] for r in es.registry(kind_class='Bot').values())
    for es in results.values()
]
average_units = sum(total_units) / len(total_units)
print("*For 3 runs:*")
print("Average weight delivered per run:", average_weight)
print("Average units delivered per run:", average_units)


    
  

