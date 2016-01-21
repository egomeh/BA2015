//===========================================================================
/*!
 * 
 *
 * \brief       Example program for hierarchical clustering.
 * 
 * 
 *
 * \author      T. Glasmachers
 * \date        2011
 *
 *
 * \par Copyright 1995-2015 Shark Development Team
 * 
 * <BR><HR>
 * This file is part of Shark.
 * <http://image.diku.dk/shark/>
 * 
 * Shark is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Shark is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Shark.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
//===========================================================================

#include <shark/Core/Shark.h>
#include <shark/Algorithms/DirectSearch/CMA.h>
#include <shark/ObjectiveFunctions/AbstractObjectiveFunction.h>
#include <iostream>
#include <string>
#include <ctime>

#include "cconfig.h"
#include "MDPTetris.h"
#include "CrossEntropy.h"

#define OPT_SEED               "-seed"
#define OPT_START_POL_FILE     "-startPolicy"
#define OPT_PIECE_FILE         "-pieceFile"
#define OPT_OPTIMIZER          "-optimizer"
#define OPT_INITIAL_SIGMA      "-sigma"

/* Number of games to play when sorting vectors for performance */
#define OPT_NB_GAMES           "-nbgames"

/* Number of games to play when asserting the performance of the mean */
#define OPT_NB_LEARNING_GAMES  "-nbLearningGames"
#define OPT_OUTPUTNAME         "-output"

/* Options for stopping criteria */
#define OPT_MAXITER            "-maxiter"
#define OPT_MAX_AGENTS         "-maxagents"

/* Noise type for the Cross Entropy */
#define OPT_NOISETYPE          "-noiseType"
#define OPT_NOISE              "-noiseValueA"
#define OPT_NOISE2             "-noiseValueB"

/* Lower bound on the step size */
#define OPT_LOWER_BOUND        "-lbound"

/* Options for regulating population and offspring */
#define OPT_LAMBDA             "-lambda"
#define OPT_OFFSPRING          "-offspring"

/* CMA-ES recombination type */
#define OPT_RECOMBINATION_TYPE "-recombinationType"

const std::string known_opts[]
        = {OPT_SEED,OPT_START_POL_FILE,OPT_PIECE_FILE,OPT_OPTIMIZER,OPT_INITIAL_SIGMA,
           OPT_NB_GAMES,OPT_NB_LEARNING_GAMES,OPT_OUTPUTNAME,OPT_MAXITER,OPT_MAX_AGENTS,
           OPT_NOISETYPE,OPT_NOISE,OPT_NOISE2,OPT_LOWER_BOUND,OPT_LAMBDA,OPT_OFFSPRING,OPT_RECOMBINATION_TYPE,
           "STOP"};

/* The stopping criteria for the experiment */
enum StoppingCriteria
{
    STOP_BY_ITERATION,
    STOP_BY_AGENTS_EVALUATED
};

/* This class is used to hand non-default experiment variables
 * to the experiment functions
 * */
template<class T>
class ExperimentOptionType
{
public:
    /* Constructor, only sets the values */
    ExperimentOptionType(bool use, T value)
    : m_use(use), m_value(value) {};

    /* Check if used */
    bool used(void) const
    { return m_use; }

    /* Get the value */
    const T &operator()(void) const
    { return m_value; }

private:
    /* Indicates if the option should be applied */
    bool m_use;

    /* If applied, apply this value */
    T    m_value;
};

void useCMA(std::string startPolicyFile,
            std::string piecesFile,
            unsigned int nbGames,
            unsigned int nbLearnGames,
            unsigned int boardWidth,
            unsigned int boardHeight,
            int randomSeed,
            ExperimentOptionType<double> initialSigma,
            unsigned int maxIterations,
            unsigned int maxAgents,
            StoppingCriteria stoppingCriteria,
            std::ostream & out,
            std::string outname,
            ExperimentOptionType<double> lowerBound,
            ExperimentOptionType<unsigned int> lambda,
            ExperimentOptionType<unsigned int> offspring,
            ExperimentOptionType<shark::CMA::RecombinationType> recombinationType)
{
    out << "Running CMA-ES with following configurations" << std::endl;
    out << "Start policy       : " << startPolicyFile << std::endl;
    out << "Pieces             : " << piecesFile << std::endl;
    out << "Game evaluations   : " << nbGames << std::endl;
    out << "Game learning games: " << nbLearnGames << std::endl;
    out << "Game board with    : " << boardWidth << std::endl;
    out << "Game board height  : " << boardHeight << std::endl;
    out << "Random seed        : " << randomSeed << std::endl;
    if (initialSigma.used())
        out << "initialSigma       : " << initialSigma() << std::endl;
    out << "MaxIterations      : " << maxIterations << std::endl;
    if (lowerBound.used())
        out << "lowerBound       : " << lowerBound() << std::endl;

    initialize_random_generator( randomSeed );
    shark::Rng::seed( randomSeed );
    shark::CMA cma;

    Game *game = new_game(0, 10, 20, 0, piecesFile.c_str(), NULL);
    GamesStatistics *stats = games_statistics_new(NULL, 10, NULL);

    MDPTetris objFun(10,20, nbGames, game, stats, startPolicyFile);
    if ( outname.size() > 0 )
    {
        objFun.setGamedataFilename(outname);
    }

    // If population size and offspring is given, special initialization needs to take place.
    if(offspring.used() && lambda.used())
    {
                                                                              // Sigma is set later on
        cma.init(objFun, objFun.proposeStartingPoint(), lambda(), offspring(), 1.0);
        cma.mu() = offspring();
    }
    // Othertwise, init as normal.
    else
    {
        cma.init(objFun);
    }

    if (recombinationType.used())
    {
        cma.recombinationType() = recombinationType();
    }

    /* Set the initial sigma */
    if (initialSigma.used())
    {
        cma.setSigma(initialSigma());
    }

    /* Set the lower bound */
    if(lowerBound.used())
    {
        cma.lowerBound() = lowerBound();
    }

    switch (cma.recombinationType())
    {
        case shark::CMA::EQUAL:
            std::cout << "Equal recombination" << std::endl;
            break;
        case shark::CMA::LINEAR:
            std::cout << "Linear recombination" << std::endl;
            break;
        case shark::CMA::SUPERLINEAR:
            std::cout << "Superlinear recombination" << std::endl;
            break;
    }

    std::cout << "populationSize   : " << cma.lambda() << std::endl;
    std::cout << "CMA prent number : " << cma.mu() << std::endl;
    std::cout << "CMA lower bound  : " << cma.lowerBound() << std::endl;

    int t = 0;
    int generation = 0;
    double bestScore = 0.0;

    bool running = true;

    /* Report header for CSV output */
    if ( outname.size() > 0 )
    {
        /* Write the header */
        std::ofstream fs;
        fs.open (outname.c_str());

        fs << "generation,agents,minScore,maxScore,meanScore,standardDeviation,stepSize,";
        for (int i = 0; i < objFun.numberOfVariables(); i++)
        {
            fs << "w" << i << ",";
        }

        for (int i = 0; i < nbLearnGames; i++)
        {
            fs << "s" << i << ",";
        }

        for (int i = 0; i < objFun.numberOfVariables()-1; i++)
        {
            fs << "e" << i << ",";
        }
        fs << "e" <<  objFun.numberOfVariables()-1 << std::endl;

        fs.close();
    }


    while (running)
    {

        cma.step(objFun);
        t += cma.lambda() * nbGames;

        if ( outname.size() > 0 )
        {

            /* Set the number of games for the learning curve */
            objFun.setNbGames(nbLearnGames);
            MDPTetris::MDPTetrisDetailedResult report = objFun.evalDetailed(cma.mean());
            objFun.setNbGames(nbGames);

            double centroid_score = report.mean();
            _DUMP(centroid_score);

            // Get a string of eignvalues
            std::stringstream _s;
            for (int i = 0; i < cma.eigenValues().size()-1; i++)
            {
                _s << cma.eigenValues()[i] << ",";
            }
            _s << cma.eigenValues()[cma.eigenValues().size()-1];

            std::ofstream fs;
            fs.open (outname.c_str(), std::ios::app);
            fs << generation << ","
               << t << ","
               << report.minScore() << ","
               << report.maxScore() << ","
               << report.mean() << ","
               << report.standardDeviation() << ","
               << cma.sigma()  << ","
               << report.printWeights(",") << ","
               << report.printScores(",") << ","
               << _s.str()
                  << std::endl;
        }

        if (TETRIS_MAX_SCORE - cma.solution().value > bestScore)
        {
            bestScore = TETRIS_MAX_SCORE - cma.solution().value;
            shark::RealVector solution = cma.solution().point;
            _DUMP(bestScore);
            _DUMP(t);
            _DUMP(solution);
        }
        generation++;
        if (stoppingCriteria == STOP_BY_ITERATION && generation > maxIterations)
        {
            running = false;
        }
        else if (stoppingCriteria == STOP_BY_AGENTS_EVALUATED && t > maxAgents)
        {
            running = false;
        }
    }


}


void useCE(std::string startPolicyFile,
           std::string piecesFile,
           unsigned int nbGames,
           unsigned int nbLearnGames,
           unsigned int boardWidth,
           unsigned int boardHeight,
           int randomSeed,
           ExperimentOptionType<double> initialVariance,
           unsigned int maxIterations,
           unsigned int maxAgents,
           StoppingCriteria stoppingCriteria,
           std::ostream & out,
           std::string outname,
           ExperimentOptionType<shark::CrossEntropy::INoiseType*> noise,
           ExperimentOptionType<unsigned int> lambda,
           ExperimentOptionType<unsigned int> offspring
)
{
    out << "Running Cross Entropy with following configurations" << std::endl;
    out << "Start policy       : " << startPolicyFile << std::endl;
    out << "Pieces             : " << piecesFile << std::endl;
    out << "Game evaluations   : " << nbGames << std::endl;
    out << "Game learning games: " << nbLearnGames << std::endl;
    out << "Game board with    : " << boardWidth << std::endl;
    out << "Game board height  : " << boardHeight << std::endl;
    out << "Random seed        : " << randomSeed << std::endl;
    if (initialVariance.used())
        out << "initialVariance: " << initialVariance() << std::endl;
    out << "MaxIterations      : " << maxIterations << std::endl;

    initialize_random_generator( randomSeed );
    shark::Rng::seed( randomSeed );
    shark::CrossEntropy ce;

    Game *game = new_game(0, 10, 20, 0, piecesFile.c_str(), NULL);
    GamesStatistics *stats = games_statistics_new(NULL, nbGames, NULL);

    MDPTetris objFun(10,20, nbGames, game, stats, startPolicyFile);
    if ( outname.size() > 0 )
    {
        objFun.setGamedataFilename(outname);
    }


    ce.init(objFun);

    ce.populationSize() = 100;
    ce.selectionSize() = 10;

    if(noise.used())
    {
        ce.setNoiseType( noise() );
    }


    if (initialVariance.used())
    {
        ce.setVariance(initialVariance());
    }

    if(lambda.used())
    {
        ce.populationSize() = lambda();
    }
    if(offspring.used())
    {
        ce.selectionSize() = offspring();
    }

    // Still need to set the initial sigma vector

    out << "Noise              : " << ce.getNoiseType().name() << std::endl;
    out << "PopulationSize     : " << ce.populationSize()      << std::endl;
    out << "SelectionSize      : " << ce.selectionSize()       << std::endl;
    out << "Variance           : " << ce.variance()            << std::endl;



    int t = 0;
    int generation = 0;
    double bestScore = 0.0;

    bool running = true;


    /* Report header for CSV output */
    if ( outname.size() > 0 )
    {
        /* Write the header */
        std::ofstream fs;
        fs.open (outname.c_str());

        fs << "generation,agents,minScore,maxScore,meanScore,standardDeviation,";
        for (int i = 0; i < objFun.numberOfVariables(); i++)
        {
            fs << "w" << i << ",";
        }

        for (int i = 0; i < nbLearnGames-1; i++)
        {
            fs << "s" << i << ",";
        }
        fs << "s" << nbLearnGames-1 << std::endl;

        fs.close();
    }

    while (running)
    {
        _DUMP(generation);
        ce.step(objFun);
        t += ce.populationSize() * nbGames;

        if ( outname.size() > 0 )
        {

            /* Set the number of games for the learning curve */
            objFun.setNbGames(nbLearnGames);
            MDPTetris::MDPTetrisDetailedResult report = objFun.evalDetailed(ce.mean());
            objFun.setNbGames(nbGames);


            std::ofstream fs;
            fs.open (outname.c_str(), std::ios::app);
            fs << generation << ","
            << t << ","
            << report.minScore() << ","
            << report.maxScore() << ","
            << report.mean() << ","
            << report.standardDeviation() << ","
            << report.printWeights(",")  << ","
            << report.printScores(",")
            << std::endl;
        }

        if (TETRIS_MAX_SCORE - ce.solution().value > bestScore)
        {
            bestScore = TETRIS_MAX_SCORE - ce.solution().value;
            shark::RealVector solution = ce.solution().point;
            _DUMP(bestScore);
            _DUMP(t);
            _DUMP(solution);
        }
        generation++;
        if (stoppingCriteria == STOP_BY_ITERATION && generation > maxIterations)
        {
            running = false;
        }
        else if (stoppingCriteria == STOP_BY_AGENTS_EVALUATED && t > maxAgents)
        {
            running = false;
        }
    }


}

int main( int argc, char ** argv ) 
{

    // Store all options in vector.
    std::vector<std::string> all_options;
    bool stopFound = false;
    int optionIndex = 0;
    while ( stopFound == false )
    {
        std::string cur_str = known_opts[optionIndex];
        if (  cur_str.compare(std::string("STOP")) == 0 )
        {
            stopFound = true;
        }
        else
        {
            all_options.push_back(known_opts[optionIndex]);
            optionIndex++;
        }
    }

    std::map<std::string, std::string> options;
    for (int i = 1; i < argc; i++)
    {
        std::string opt       = argv[i];
        std::string delimiter = "=";

        size_t del_pos =  opt.find(delimiter);
        if (del_pos == std::string::npos || del_pos+1 == opt.size())
        {
            std::cerr << "Ill posed command line argument: " << argv[i] << std::endl;
            return 64;
        }

        std::string key       = opt.substr(0, opt.find(delimiter));
        std::string value     = opt.substr(opt.find(delimiter)+1, opt.size());
        options[key] = value;
        std::cout << "Registering option " << key << " : " << value << std::endl;
        // Make sure that we know of the option.
        if (std::find(all_options.begin(), all_options.end(), key) == all_options.end())
        {
            std::cerr << "Option is unknown, aborting! " << key << std::endl;
            return 64;
        }
    }


    /* Set the random seed for the gsl random generator */
    int seed;
    if (options.count(OPT_SEED) == 1)
    {
       seed = atoi ( options[OPT_SEED].c_str() );
    }
    else
    {
        seed = (int) time(0);
    }


    /* Set the initial policy file */
    std::string start_policy;
    if (options.count(OPT_START_POL_FILE) == 1)
    {
        start_policy = MDPTETRIS_DATA_PATH(options[OPT_START_POL_FILE]);
    }
    else
    {
        start_policy = MDPTETRIS_DATA_PATH("starting_policy01.dat");
    }


    /* Set the initial policy file */
    std::string piece_file;
    if (options.count(OPT_PIECE_FILE) == 1)
    {
        piece_file = MDPTETRIS_DATA_PATH(options[OPT_PIECE_FILE]);
    }
    else
    {
        piece_file = MDPTETRIS_DATA_PATH("pieces4.dat");
    }

    /* Optionally set the initial Sigma
     * For Cross entropy, this sets all the variance
     * intries to the value given.
     * In CMA this is the initial step-size.
     * */
    ExperimentOptionType<double> initialSigma(false, 1.0);
    if (options.count(OPT_INITIAL_SIGMA) == 1)
    {
        double s = atof ( options[OPT_INITIAL_SIGMA].c_str() );
        initialSigma = ExperimentOptionType<double>(true, s);
    }

    /* CMA specific lower bound.
     * The will force the CMA step-size to keep
     * the smallest eignvalue of the covariance matrix
     * multiplied by the step-size to stay above this
     * value.
     * */
    ExperimentOptionType<double> lowerBound(false, 1E-20);
    if (options.count(OPT_LOWER_BOUND) == 1)
    {
        double s = atof ( options[OPT_LOWER_BOUND].c_str() );
        lowerBound = ExperimentOptionType<double>(true, s);
    }


    /* Register option for population size */
    ExperimentOptionType<unsigned int> lambda(false, 100);
    if (options.count(OPT_LAMBDA) == 1)
    {
        int i = atoi( options[OPT_LAMBDA].c_str() );
        lambda = ExperimentOptionType<unsigned int>(true, i);
    }

    /* Register option for offspriong size */
    ExperimentOptionType<unsigned int> offspring(false, 100);
    if (options.count(OPT_OFFSPRING) == 1)
    {
        int i = atoi( options[OPT_OFFSPRING].c_str() );
        offspring = ExperimentOptionType<unsigned int>(true, i);
    }


    unsigned int nbGames = 1;
    if (options.count(OPT_NB_GAMES) == 1)
    {
        nbGames = atoi ( options[OPT_NB_GAMES].c_str() );
    }

    unsigned int nbLearnGames = 30;
    if (options.count(OPT_NB_LEARNING_GAMES) == 1)
    {
        nbLearnGames = atoi ( options[OPT_NB_LEARNING_GAMES].c_str() );
    }

    /* Cross Entropy specific for noise type */
    double noiseVal = 0;
    if (options.count(OPT_NOISE) == 1)
    {
        noiseVal = atof( options[OPT_NOISE].c_str() );
    }
    double noiseVal2 = 0;
    if (options.count(OPT_NOISE2) == 1)
    {
        noiseVal2 = atof( options[OPT_NOISE2].c_str() );
    }

    //shark::CrossEntropy::SamplingNoise noiseType = shark::CrossEntropy::NONE;
    ExperimentOptionType<shark::CrossEntropy::INoiseType*> noise(false, new shark::CrossEntropy::INoiseType());
    if (options.count(OPT_NOISETYPE) == 1)
    {
        switch (atoi( options[OPT_NOISETYPE].c_str() ))
        {
            case 0:
            {
                // The standard noise type is no noise. This renders any changes redundant.
                break;
            }
            case 1:
            {
                noise = ExperimentOptionType<shark::CrossEntropy::INoiseType*>
                         (true, new shark::CrossEntropy::ConstantNoise(noiseVal));
                break;
            }
            case 2:
            {
                noise = ExperimentOptionType<shark::CrossEntropy::INoiseType*>
                        (true, new shark::CrossEntropy::LinearNoise(noiseVal, -noiseVal2));
                break;
            }
            default:
            {
                std::cerr << "Noise not recognized" << std::endl;
                exit(-1);
            }
        }
    }

    /* Register option for recombination type */
    ExperimentOptionType<shark::CMA::RecombinationType> recombinationType(false, shark::CMA::EQUAL);
    if (options.count(OPT_RECOMBINATION_TYPE) == 1)
    {
        int i = atoi( options[OPT_RECOMBINATION_TYPE].c_str() );
        switch (i){
            case 0:
            {
                recombinationType = ExperimentOptionType<shark::CMA::RecombinationType> (true, shark::CMA::EQUAL);
                break;
            }
            case 1:
            {
                recombinationType = ExperimentOptionType<shark::CMA::RecombinationType> (true, shark::CMA::LINEAR);
                break;
            }
            case 2:
            {
                recombinationType = ExperimentOptionType<shark::CMA::RecombinationType> (true, shark::CMA::SUPERLINEAR);
                break;
            }
            default:
            {
                std::cerr << "Invalid option for recombination type" << std::endl;
            }

        }
    }


    unsigned int boardWidth = 10;
    unsigned int boardHeight = 20;

    StoppingCriteria stoppingCriteria = STOP_BY_ITERATION;
    unsigned int maxIterations = 80;  /* Default stop at 80 iterations */
    unsigned int maxAgents = 80000;   /* Default agents to evaluate is 80000 */
    if (options.count(OPT_MAXITER) == 1)
    {
        maxIterations = atoi ( options[OPT_MAXITER].c_str() );
        stoppingCriteria = STOP_BY_ITERATION;
    }
    else if (options.count(OPT_MAX_AGENTS) == 1)
    {
        maxAgents = atoi ( options[OPT_MAX_AGENTS].c_str() );
        stoppingCriteria = STOP_BY_AGENTS_EVALUATED;
    }

    std::string outputfile = std::string("");
    if (options.count(OPT_OUTPUTNAME) == 1)
    {
        outputfile = std::string(options[OPT_OUTPUTNAME]) + ".txt";
    }

    if ( options.count(OPT_OPTIMIZER) == 1 )
    {
        if ( options[OPT_OPTIMIZER].compare("cma") == 0 )
        {
            useCMA(
                    start_policy,
                    piece_file,
                    nbGames,
                    nbLearnGames,
                    boardWidth,
                    boardHeight,
                    seed,
                    initialSigma,
                    maxIterations,
                    maxAgents,
                    stoppingCriteria,
                    std::cout,
                    outputfile,
                    lowerBound,
                    lambda,
                    offspring,
                    recombinationType
            );
        }
        else if ( options[OPT_OPTIMIZER].compare("ce") == 0 )
        {
            useCE(
                    start_policy,
                    piece_file,
                    nbGames,
                    nbLearnGames,
                    boardWidth,
                    boardHeight,
                    seed,
                    initialSigma,
                    maxIterations,
                    maxAgents,
                    stoppingCriteria,
                    std::cout,
                    outputfile,
                    noise,
                    lambda,
                    offspring
            );
        }
    }
    else
    {
        std::cerr << "Optimizer not recognized!" << std::endl;
        return 64;
    }

}





