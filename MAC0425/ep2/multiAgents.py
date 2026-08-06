# multiAgents.py
# --------------

# Nome: Laufernando Souza Dias
# NUSP: 11222947

# Licensing Information:  You are free to use or extend these projects for
# educational purposes provided that (1) you do not distribute or publish
# solutions, (2) you retain this notice, and (3) you provide clear
# attribution to UC Berkeley, including a link to http://ai.berkeley.edu.
# 
# Attribution Information: The Pacman AI projects were developed at UC Berkeley.
# The core projects and autograders were primarily created by John DeNero
# (denero@cs.berkeley.edu) and Dan Klein (klein@cs.berkeley.edu).
# Student side autograding was added by Brad Miller, Nick Hay, and
# Pieter Abbeel (pabbeel@cs.berkeley.edu).


from util import manhattanDistance
from game import Directions
import random, util

from game import Agent
from pacman import GameState

class ReflexAgent(Agent):
    """
    A reflex agent chooses an action at each choice point by examining
    its alternatives via a state evaluation function.

    The code below is provided as a guide.  You are welcome to change
    it in any way you see fit, so long as you don't touch our method
    headers.
    """

    def __init__(self, evalFn = 'scoreEvaluationFunction'):
        self.evaluationFunction = util.lookup(evalFn, globals())

    def getAction(self, gameState: GameState):
        """
        You do not need to change this method, but you're welcome to.

        getAction chooses among the best options according to the evaluation function.

        Just like in the previous project, getAction takes a GameState and returns
        some Directions.X for some X in the set {NORTH, SOUTH, WEST, EAST, STOP}
        """
        # Collect legal moves and successor states
        legalMoves = gameState.getLegalActions()

        # Choose one of the best actions
        successors = [(gameState.generatePacmanSuccessor(action)) for action in legalMoves]
        scores = [self.evaluationFunction(succesor) for successors in successors]
        bestScore = max(scores)
        bestIndices = [index for index in range(len(scores)) if scores[index] == bestScore]
        chosenIndex = random.choice(bestIndices) # Pick randomly among the best
        return legalMoves[chosenIndex]

def scoreEvaluationFunction(currentGameState: GameState):
    """
    This default evaluation function just returns the score of the state.
    The score is the same one displayed in the Pacman GUI.

    This evaluation function is meant for use with adversarial search agents
    (not reflex agents).
    """
    return currentGameState.getScore()

class MultiAgentSearchAgent(Agent):
    """
    This class provides some common elements to all of your
    multi-agent searchers.  Any methods defined here will be available
    to the MinimaxPacmanAgent, AlphaBetaPacmanAgent & ExpectimaxPacmanAgent.

    You *do not* need to make any changes here, but you can if you want to
    add functionality to all your adversarial search agents.  Please do not
    remove anything, however.

    Note: this is an abstract class: one that should not be instantiated.  It's
    only partially specified, and designed to be extended.  Agent (game.py)
    is another abstract class.
    """

    def __init__(self, evalFn = 'scoreEvaluationFunction', depth = '2'):
        self.index = 0 # Pacman is always agent index 0
        self.evaluationFunction = util.lookup(evalFn, globals())
        self.depth = int(depth)

class MinimaxAgent(MultiAgentSearchAgent):
    """
    Your minimax agent (question 1)
    """

    def minimax(self, agentIndex, gameState: GameState, depth: int):
        # Condições terminais: profundidade esgotada ou estado final
        if depth == 0 or gameState.isWin() or gameState.isLose():
            return self.evaluationFunction(gameState), None

        numAgents = gameState.getNumAgents()
        legalActions = gameState.getLegalActions(agentIndex)

        # Próximo agente: depois do último fantasma, volta ao Pac-Man
        nextAgent = (agentIndex + 1) % numAgents

        # Só decrementa profundidade quando o Pac-Man (agente 0) volta a jogar
        nextDepth = depth - 1 if nextAgent == 0 else depth

        # Pac-Man é o maximizador (agentIndex == 0)
        if agentIndex == 0:
            bestScore = float('-inf')
            bestAction = None
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.minimax(nextAgent, successor, nextDepth)
                if score > bestScore:
                    bestScore = score
                    bestAction = action
            return bestScore, bestAction

        # Fantasmas são minimizadores (agentIndex >= 1)
        else:
            bestScore = float('inf')
            bestAction = None
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.minimax(nextAgent, successor, nextDepth)
                if score < bestScore:
                    bestScore = score
                    bestAction = action
            return bestScore, bestAction

    def getAction(self, gameState: GameState):
        """
        Returns the minimax action from the current gameState using self.depth
        and self.evaluationFunction.

        Here are some method calls that might be useful when implementing minimax.

        gameState.getLegalActions(agentIndex):
        Returns a list of legal actions for an agent
        agentIndex=0 means Pacman, ghosts are >= 1

        gameState.generateSuccessor(agentIndex, action):
        Returns the successor game state after an agent takes an action

        gameState.getNumAgents():
        Returns the total number of agents in the game

        gameState.isWin():
        Returns whether or not the game state is a winning state

        gameState.isLose():
        Returns whether or not the game state is a losing state
        """
        _, action = self.minimax(0, gameState, self.depth)
        return action

    
class AlphaBetaAgent(MultiAgentSearchAgent):
    """
    Your minimax agent with alpha-beta pruning (question 2)
    """

    def alphabeta(self, agentIndex, gameState: GameState, depth: int,
                  alpha: float, beta: float):
        # Condições terminais: profundidade esgotada ou estado final
        if depth == 0 or gameState.isWin() or gameState.isLose():
            return self.evaluationFunction(gameState), None

        numAgents = gameState.getNumAgents()
        legalActions = gameState.getLegalActions(agentIndex)

        # Próximo agente e profundidade (mesma lógica do minimax)
        nextAgent = (agentIndex + 1) % numAgents
        nextDepth = depth - 1 if nextAgent == 0 else depth

        # Pac-Man é o maximizador (agentIndex == 0)
        if agentIndex == 0:
            bestScore = float('-inf')
            bestAction = None
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.alphabeta(nextAgent, successor, nextDepth, alpha, beta)
                if score > bestScore:
                    bestScore = score
                    bestAction = action
                # Atualiza alpha com o melhor valor encontrado para o max
                if bestScore > alpha:
                    alpha = bestScore
                # Poda: corta se estritamente maior que beta 
                if bestScore > beta:
                    break
            return bestScore, bestAction

        # Fantasmas são minimizadores (agentIndex >= 1)
        else:
            bestScore = float('inf')
            bestAction = None
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.alphabeta(nextAgent, successor, nextDepth, alpha, beta)
                if score < bestScore:
                    bestScore = score
                    bestAction = action
                # Atualiza beta com o melhor valor encontrado para o min
                if bestScore < beta:
                    beta = bestScore
                # Poda: corta se estritamente menor que alpha 
                if bestScore < alpha:
                    break
            return bestScore, bestAction

    def getAction(self, gameState: GameState):
        # Chama com alpha e beta nos extremos — nenhum corte no início
        _, action = self.alphabeta(0, gameState, self.depth,
                                   float('-inf'), float('inf'))
        return action

class ExpectimaxAgent(MultiAgentSearchAgent):
    """
    Your expectimax agent (question 3)
    """

    def expectimax(self, agentIndex, gameState: GameState, depth: int):
        # Condições terminais: profundidade esgotada ou estado final
        if depth == 0 or gameState.isWin() or gameState.isLose():
            return self.evaluationFunction(gameState), None

        numAgents = gameState.getNumAgents()
        legalActions = gameState.getLegalActions(agentIndex)

        # Próximo agente e profundidade (mesma lógica dos anteriores)
        nextAgent = (agentIndex + 1) % numAgents
        nextDepth = depth - 1 if nextAgent == 0 else depth

        # Pac-Man é o maximizador (agentIndex == 0)
        if agentIndex == 0:
            bestScore = float('-inf')
            bestAction = None
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.expectimax(nextAgent, successor, nextDepth)
                if score > bestScore:
                    bestScore = score
                    bestAction = action
            return bestScore, bestAction

        # Fantasmas são nós de chance: retornam a média dos filhos
        else:
            totalScore = 0
            for action in legalActions:
                successor = gameState.generateSuccessor(agentIndex, action)
                score, _ = self.expectimax(nextAgent, successor, nextDepth)
                totalScore += score
            expectedScore = totalScore / len(legalActions)
            return expectedScore, None

    def getAction(self, gameState: GameState):
        _, action = self.expectimax(0, gameState, self.depth)
        return action

def betterEvaluationFunction(currentGameState: GameState):
    pos = currentGameState.getPacmanPosition()
    food = currentGameState.getFood().asList()
    ghosts = currentGameState.getGhostStates()
    score = currentGameState.getScore()
    # Incentive to get food
    if food:
        # Minimize distanace to closest food
        minFoodDist = min(manhattanDistance(pos, f) for f in food)
        score += 10.0 / (minFoodDist + 1)
        # Minimize remaining food
        score -= 4 * len(food)
    # Avoid ghosts
    for ghost in ghosts:
        ghostPos = ghost.getPosition()
        dist = manhattanDistance(pos, ghostPos)
        # Get closer to scared ghosts
        if ghost.scaredTimer > 0:
            score += 20.0 / (dist + 1)
        else:
            if dist <= 1:
                # Avoid immediate death
                score -= 500
            else:
                # Get away from ghosts
                score -= 2.0 / dist
    return score

# Abbreviation
better = betterEvaluationFunction
