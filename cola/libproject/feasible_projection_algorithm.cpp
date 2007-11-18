/**
 * @file 
 * @brief Solve an instance of the "Variable Placement with Separation
 * Constraints" problem, that is a projection onto separation constraints,
 * whilest always maintaining feasibility.
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2007 Authors
 *
 * Released under GNU LGPL.  Read the file 'COPYING' for more information.
 */

#include <vector>
#include <numeric>
#include <cmath>
#include <cfloat>
#include "util.h"
#include "feasible_projection_algorithm.h"

namespace algorithm {

Constraint::Constraint(Variable *l, Variable *r, const double g)
    : l(l), r(r), g(g)
    , active(false) 
    , lm(0)
{
    l->out.push_back(this);
    r->in.push_back(this);
}

Block::Block(Variable* v) 
    : X(v->d)
    , XI(v->x)
{
    V.push_back(v);
    v->b=0;
    v->block=this;
}

struct sum_posdiff {
    double operator() (double acc, Variable const *v) {
        return acc + (v->d - v->b);
    }
};
/**
 * Compute the optimal position for this block based on the ideal positions of
 * its constituent variables.  
 * That is, for each variable in the block \f$v_i\in V\f$ with ideal
 * positions \f$d_i\f$ and offset relative to the block reference position
 * \f$b_i\f$ the ideal position for the block is
 * \f$\frac{1}{|V|}\sum_{v_i\in V} d_i - b_i\f$.
 */
double Block::optBlockPos() const {
    return accumulate(V.begin(),V.end(),0.0,sum_posdiff()) / V.size();
}

double Block::compute_dfdv(Variable const* v, Constraint const* last) {
    double dfdv = 2.0 * (v->x - v->d);
    for(Constraints::const_iterator i=v->out.begin();i!=v->out.end();i++) {
        Constraint *c=*i;
        if(c!=last && c->active) {
            c->lm = compute_dfdv(c->r,c);
            dfdv += c->lm;
        }
    }
    for(Constraints::const_iterator i=v->in.begin();i!=v->in.end();i++) {
        Constraint *c=*i;
        if(c!=last && c->active) {
            c->lm = -compute_dfdv(c->l,c);
            dfdv -= c->lm;
        }
    }
    return dfdv;
}
struct reset_lm { void operator() (Constraint *c) { c->lm = 0; } };
/**
 */
void Block::computeLagrangians() {
    for_each(C.begin(),C.end(),reset_lm());
    compute_dfdv(V[0],NULL);
}

FeasibleProjectionAlgorithm::
FeasibleProjectionAlgorithm(
        std::vector<Variable*> const &vs, 
        std::vector<Constraint *> const &cs) 
    : vs(vs)
    , cs(cs)
    , inactive(cs.begin(),cs.end()) 
{

}
void FeasibleProjectionAlgorithm::
init_blocks() {
    for(Variables::const_iterator i=vs.begin();i!=vs.end();i++) {
        blocks.push_back(new Block(*i));
    }
}

FeasibleProjectionAlgorithm::
~FeasibleProjectionAlgorithm() {
    for_each(blocks.begin(),blocks.end(),delete_object());
}
struct MaxSafeAlpha {
    MaxSafeAlpha(Constraint *&c, double &alpha) : safeC(c), safeAlpha(alpha) { 
        c=NULL;
        safeAlpha = DBL_MAX;
    }
    void operator()(Constraint * c) {
        double alpha = 0;
        double Xl = c->l->block->X,
               Xr = c->r->block->X,
               bl = c->l->b,
               br = c->r->b;
        if(Xl + bl + c->g <= Xr + br) {
            alpha = 1;
        } else {
            double XIl = c->l->block->XI,
                   XIr = c->r->block->XI;
            double Al = XIl + bl,
                   Ar = XIr + br,
                   Bl = Xl - XIl,
                   Br = Xr - XIr;
            alpha = (c->g + Al - Ar) / (Br - Bl);
        }
        if(alpha < safeAlpha) {
            safeC = c;
            safeAlpha = alpha;
        }
    }
    Constraint *&safeC;
    double &safeAlpha;
};
void FeasibleProjectionAlgorithm:: 
make_optimal() {
    Constraint *c;
    double alpha;
    for_each(inactive.begin(),inactive.end(),MaxSafeAlpha(c,alpha));
    while(alpha < 1) {
        make_active(c, alpha);
        inactive.erase(c);
        for_each(inactive.begin(),inactive.end(),MaxSafeAlpha(c,alpha));
    }
    for(vector<Block*>::iterator i=blocks.begin(); i!=blocks.end(); i++) {
        Block* b=*i;
        b->XI = b->X;
    }
}
void FeasibleProjectionAlgorithm:: 
make_active(Constraint *c, double alpha) {
}
void FeasibleProjectionAlgorithm:: 
make_inactive(Constraint *c) {
}
void FeasibleProjectionAlgorithm:: 
split_blocks() {
}

} // namespace algorithm
/*
 * vim: set cindent 
 * vim: ts=4 sw=4 et tw=0 wm=0
 */