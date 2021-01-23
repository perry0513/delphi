#include "oracle_interface.h"
#include "oracle_solver.h"
#include "../expr2sygus.h"
#include <solvers/smt2/smt2_dec.h>
#include <langapi/language_util.h>
#include <util/replace_expr.h>
#include <iostream>



void oracle_interfacet::add_problem(const problemt &problem, const solutiont &solution, oracle_solvert &solver)
{
  // add verification problem "/exists x /neg (/alpha \implies \phi"
  verify_encoding.clear();
  verify_encodingt::check_function_bodies(solution.functions);
  verify_encoding.functions = solution.functions;
  verify_encoding.free_variables = problem.free_variables;
  
  const exprt encoded_constraints = (problem.assumptions.size()>1)?
      verify_encoding(implies_exprt(conjunction(problem.assumptions),conjunction(problem.constraints))): 
      verify_encoding(conjunction(problem.constraints));

  solver.set_to_false(encoded_constraints);
}


std::vector<std::vector<exprt>> find_synth_fun(const exprt &expr, const problemt &problem)
{
  std::vector<std::vector<exprt>> result;

  if(expr.id()==ID_mathematical_function)
  {
    auto tmp=to_function_application_expr(expr);
    if(problem.synthesis_functions.find(tmp.function().id())!=problem.synthesis_functions.end())
    {
      result.push_back(tmp.arguments());
    }
  }
  return result;
}

void oracle_interfacet::replace_oracles(exprt &expr, const problemt &problem, oracle_solvert &solver)
{
  for(auto &op: expr.operands())
    replace_oracles(op, problem, solver);

  if(expr.id()==ID_function_application)
  {
    auto &func_app = to_function_application_expr(expr);
    if(problem.oracle_symbols.find(to_symbol_expr(func_app.function()).get_identifier())!=problem.oracle_symbols.end())
    {
     exprt result = solver.get_oracle_value(func_app, func_app.arguments());
     if(result!=nil_exprt())
      expr = result;
    }
  }
}


void oracle_interfacet::build_counterexample_constraint(oracle_solvert &solver, 
    const counterexamplet &counterexample, problemt &problem)
{ 
  counterexamplet result;
  // get counterexample
  for(const auto &var : problem.free_variables)
  {
    exprt value=solver.get(var);
    result.assignment[var]=value;
    if(value==nil_exprt() && var.id()==ID_nondet_symbol)
    {
      nondet_symbol_exprt tmp_var=to_nondet_symbol_expr(var);
      tmp_var.set_identifier("nondet_"+id2string(to_nondet_symbol_expr(var).get_identifier()));
      value=solver.get(tmp_var);
      result.assignment[var]=value;
    }
    if(value==nil_exprt())
    {
      std::cout << "Warning: unable to find value for "<< var.pretty()<<std::endl;
      result.assignment[var] = constant_exprt("0", var.type());
      std::cout<<"Assume has been simplified out by solver.\n" <<std::endl;
    }
  }

  // make constraint from counterexample and problem
  std::vector<exprt> new_synthesis_constraints;
  for(auto &p: problem.constraints)
  {
    exprt synthesis_constraint = p;
    replace_expr(result.assignment, synthesis_constraint);
    replace_oracles(synthesis_constraint, problem, solver);
    new_synthesis_constraints.push_back(synthesis_constraint);
  }   
  problem.synthesis_constraints.push_back(and_exprt(new_synthesis_constraints));
}

void oracle_interfacet::call_oracles(problemt &problem, 
const solutiont &solution, const counterexamplet &counterexample, oracle_solvert &solver)
{
  // call the other oracles here

  // for all problem.oracle_constraint_gen
  // call the oracle and add the constraints to problem.synthesis_constraints

  // for all problem.oracle_assumption_gen
  // call the oracle and add the assumptions to problem.assumptions
}

oracle_interfacet::resultt oracle_interfacet::operator()(problemt &problem,
                                                         const solutiont &solution)
{
  // get solver
  smt2_dect subsolver(
      ns, "fastsynth", "generated by fastsynth",
      "LIA", smt2_dect::solvert::Z3, message_handler);
   oracle_solvert oracle_solver(subsolver, message_handler);
   oracle_solver.oracle_fun_map=&problem.oracle_symbols; 

  return this->operator()(problem, solution, oracle_solver);
}

oracle_interfacet::resultt oracle_interfacet::operator()(problemt &problem,
    const solutiont &solution,
    oracle_solvert &solver)
  {

    add_problem(problem, solution, solver);
    decision_proceduret::resultt result = solver();

    switch(result)
    {
      case decision_proceduret::resultt::D_SATISFIABLE:
      {
        counterexample = verify_encoding.get_counterexample(solver);
        build_counterexample_constraint(solver, counterexample, problem);
        call_oracles(problem, solution, counterexample, solver);
        return oracle_interfacet::resultt::FAIL; 
      }
      case decision_proceduret::resultt::D_UNSATISFIABLE:
      {
        counterexample.clear();
        return oracle_interfacet::resultt::PASS;
      }

      case decision_proceduret::resultt::D_ERROR:
      default:
      {
        std::cout<<"ERROR oracle interface\n";
        counterexample=
        verify_encoding.get_counterexample(solver);
        call_oracles(problem, solution, counterexample, solver);
        return oracle_interfacet::resultt::FAIL;
      } 
    }
  }

counterexamplet oracle_interfacet::get_counterexample()
{
  return counterexample;
}  