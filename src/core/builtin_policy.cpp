#include <despot/core/builtin_policy.h>
#include <despot/interface/pomdp.h>
#include <unistd.h>

using namespace std;

namespace despot {

/* =============================================================================
 * BlindPolicy class
 * =============================================================================*/

BlindPolicy::BlindPolicy(const DSPOMDP* model, int action, ParticleLowerBound* 
	bound, Belief* belief) :
	DefaultPolicy(model, bound, belief),
	action_(action) {
}

int BlindPolicy::Action(const vector<State*>& particles, RandomStreams& streams,
	History& history) const {
	return action_;
}

ValuedAction BlindPolicy::Search() {
	double dummy_value = Globals::NEG_INFTY;
	return ValuedAction(action_, dummy_value);
}

void BlindPolicy::Update(int action, OBS_TYPE obs) {
}

/* =============================================================================
 * RandomPolicy class
 * =============================================================================*/

RandomPolicy::RandomPolicy(const DSPOMDP* model, ParticleLowerBound* bound,
	Belief* belief) :
	DefaultPolicy(model, bound, belief) {
}

RandomPolicy::RandomPolicy(const DSPOMDP* model,
	const vector<double>& action_probs,
	ParticleLowerBound* bound, Belief* belief) :
	DefaultPolicy(model, bound, belief),
	action_probs_(action_probs) {
	double sum = 0;
	for (int i = 0; i < action_probs.size(); i++)
		sum += action_probs[i];
	assert(fabs(sum - 1.0) < 1.0e-8);
}

int RandomPolicy::Action(const vector<State*>& particles,
	RandomStreams& streams, History& history) const {
	if (action_probs_.size() > 0) {
		return Random::GetCategory(action_probs_, Random::RANDOM.NextDouble());
	} else {
		return Random::RANDOM.NextInt(model_->NumActions());
	}
}

ValuedAction RandomPolicy::Search() {
	double dummy_value = Globals::NEG_INFTY;
	if (action_probs_.size() > 0) {
		return ValuedAction(
			Random::GetCategory(action_probs_, Random::RANDOM.NextDouble()),
			dummy_value);
	} else {
		return ValuedAction(Random::RANDOM.NextInt(model_->NumActions()),
			dummy_value);
	}
}

void RandomPolicy::Update(int action, OBS_TYPE obs) {
}

/* =============================================================================
 * MajorityActionPolicy class
 * =============================================================================*/

MajorityActionPolicy::MajorityActionPolicy(const DSPOMDP* model,
	const StatePolicy& policy, ParticleLowerBound* bound, Belief* belief) :
	DefaultPolicy(model, bound, belief),
	policy_(policy) {
}

int MajorityActionPolicy::Action(const vector<State*>& particles,
	RandomStreams& streams, History& history) const {
	vector<double> frequencies(model_->NumActions());

	for (int i = 0; i < particles.size(); i++) {
		State* particle = particles[i];
		int action = policy_.GetAction(*particle);
		frequencies[action] += particle->weight;
	}

	int bestAction = 0;
	double bestWeight = frequencies[0];
	for (int a = 1; a < frequencies.size(); a++) {
		if (bestWeight < frequencies[a]) {
			bestWeight = frequencies[a];
			bestAction = a;
		}
	}

	return bestAction;
}

/* =============================================================================
 * ModeStatePolicy class
 * =============================================================================*/

ModeStatePolicy::ModeStatePolicy(const DSPOMDP* model,
	const StateIndexer& indexer, const StatePolicy& policy,
	ParticleLowerBound* bound, Belief* belief) :
	DefaultPolicy(model, bound, belief),
	indexer_(indexer),
	policy_(policy) {
	state_probs_.resize(indexer_.NumStates());
}

int ModeStatePolicy::Action(const vector<State*>& particles,
	RandomStreams& streams, History& history) const {
	double maxWeight = 0;
	State* mode = NULL;
	for (int i = 0; i < particles.size(); i++) {
		State* particle = particles[i];
		int id = indexer_.GetIndex(particle);
		state_probs_[id] += particle->weight;

		if (state_probs_[id] > maxWeight) {
			maxWeight = state_probs_[id];
			mode = particle;
		}
	}

	for (int i = 0; i < particles.size(); i++) {
		state_probs_[indexer_.GetIndex(particles[i])] = 0;
	}

	assert(mode != NULL);
	return policy_.GetAction(*mode);
}

/* =============================================================================
 * MMAPStatePolicy class
 * =============================================================================*/

MMAPStatePolicy::MMAPStatePolicy(const DSPOMDP* model,
	const MMAPInferencer& inferencer, const StatePolicy& policy,
	ParticleLowerBound* bound, Belief* belief) :
	DefaultPolicy(model, bound, belief),
	inferencer_(inferencer),
	policy_(policy) {
}

int MMAPStatePolicy::Action(const vector<State*>& particles,
	RandomStreams& streams, History& history) const {
	return policy_.GetAction(*inferencer_.GetMMAP(particles));
}

} // namespace despot
