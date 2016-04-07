/* Copyright (C) 2014 - 2015  James Balamuta, Stephane Guerrier, Roberto Molinari
 *
 * This file is part of GMWM R Methods Package
 *
 * The `gmwm` R package is free software: you can redistribute it and/or modify it
 * under the terms of the Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
 * as the LICENSE file.
 *
 * The `gmwm` R package is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the Attribution-NonCommercial-ShareAlike 4.0 International 
 * (CC BY-NC-SA 4.0) along with `gmwm`.  If not, see <http://www.smac-group.com/licensing/>.
 * 
 */

#include <RcppArmadillo.h>

#include "gen_process.h"

// Need to have access to diff_cpp
#include "rtoarmadillo.h"

// For invertibility check in ARMA
#include "ts_checks.h"

/* ------------------------------ Start Process Generation Functions ------------------------------ */

//' Generate a White Noise Process (\eqn{WN(\sigma^2)})
//' Generates a White Noise Process with variance parameter \eqn{\sigma ^2}.
//' @param N      An \code{integer} for signal length.
//' @param sigma2 A \code{double} that contains process variance.
//' @return wn A \code{vec} containing the white noise.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_wn(10, 1.5)
// [[Rcpp::export]]
arma::vec gen_wn(const unsigned int N, const double sigma2 = 1)
{
  arma::vec wn(N);
  double sigma = sqrt(sigma2);
  for(unsigned int i = 0; i < N; i++){
      wn(i) = R::rnorm(0.0, sigma);
  }

	return wn;
}

//' Generate a Drift Process
//' 
//' Generates a Drift Process with a given slope, \eq{\omega}.
//' @param N     An \code{integer} for signal length.
//' @param slope A \code{double} that contains drift slope
//' @return A \code{vec} containing the drift.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_dr(10, 8.2)
// [[Rcpp::export]]
arma::vec gen_dr(const unsigned int N, const double slope = 5)
{
  arma::vec gd(N);
  gd.fill(slope);
	return cumsum(gd);
}

//' Generate a Quantisation Noise (QN) sequence
//' 
//' Generate an QN sequence given \eqn{Q^2}
//' @param N  An \code{integer} for signal length.
//' @param q2 A \code{double} that contains autocorrection.
//' @return A \code{vec} containing the QN process.
//' @keywords internal
//' @details 
//' To generate the quantisation noise, we follow this recipe:
//' First, we generate using a random uniform distribution:
//' \deqn{U_k^*\sim U\left[ {0,1} \right]}{U_k^*~U[0,1]}
//' 
//' Then, we multiple the sequence by \eqn{\sqrt{12}}{sqrt(12)} so:
//' \deqn{{U_k} = \sqrt{12} U_k^*}{U_k = sqrt(12)*U_k^*}
//' 
//' Next, we find the derivative of \eqn{{U_k}}{U_k}
//' \deqn{{{\dot U}_k} = \frac{{{U_{k + \Delta t}} - {U_k}}}{{\Delta t}}}{U_k^. = (U_(k + (delta)t) - U_k)}
//'
//' In this case, we modify the derivative such that:
//' \eqn{{{\dot U}_k}\Delta t = {U_{k + \Delta t}} - {U_k}}{U_k^. * (delta)t = U_{k + (delta)*t} - U_k}
//'
//' Thus, we end up with:
//' \deqn{{x_k} = \sqrt Q {{\dot U}_k}\Delta t}{x_k = sqrt(Q)*U_k^.*(delta)t}
//' \deqn{{x_k} = \sqrt Q \left( {{U_{k + 1}} - {U_k}} \right)}{x_k = sqrt(Q)* (U_(k+1) - U_(k))}
//'
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @examples
//' gen_qn(10, 5)
// [[Rcpp::export]]
arma::vec gen_qn(const unsigned int N, double q2 = .1)
{
  double sqrt12 = sqrt(12);
  
  arma::vec gu(N+1);
  
	for(unsigned int i=0; i <= N; i++ )
	{		
		gu(i) = sqrt12*R::runif(0.0,1.0);
	}

	return sqrt(q2)*diff_cpp(gu);
}


//' Generate an Autoregressive Order 1 ( AR(1) ) sequence
//' Generate an Autoregressive Order 1 sequence given \eqn{\phi} and \eqn{\sigma^2}.
//' @param N      An \code{unsigned integer} for signal length.
//' @param phi    A \code{double} that contains autocorrection.
//' @param sigma2 A \code{double} that contains process variance.
//' @return A \code{vec} containing the AR(1) process.
//' @details
//' The function implements a way to generate the AR(1)'s \eqn{x_t}{x[t]} values without calling the general ARMA function.
//' The autoregressive order 1 process is defined as \eqn{{x_t} = {\phi _1}{x_{t - 1}} + {w_t} }{x[t] = phi[1]x[t-1]  + w[t]},
//'  where \eqn{{w_t}\mathop  \sim \limits^{iid} N\left( {0,\sigma _w^2} \right)}{w[t] ~ N(0,sigma^2) iid}
//' 
//' The function first generates a vector of white noise using \code{\link[gmwm]{gen_wn}} and then obtains the
//' AR values under the above equation.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_ar1(10, 5, 1.2)
// [[Rcpp::export]]
arma::vec gen_ar1(const unsigned int N, const double phi = .3, const double sigma2 = 1)
{

	arma::vec wn = gen_wn(N+1, sigma2);
	arma::vec gm = arma::zeros<arma::vec>(N+1);
	for(unsigned int i=1; i <= N; i++ )
	{		
		gm(i) = phi*gm(i-1) + wn(i);
	}

	return gm.rows(1,N);
}

//' Generate a Random Walk without Drift
//' 
//' Generates a random walk without drift.
//' @param N      An \code{integer} for signal length.
//' @param sigma2 A \code{double} that contains process variance.
//' @return grw A \code{vec} containing the random walk without drift.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_rw(10, 8.2)
// [[Rcpp::export]]
arma::vec gen_rw(const unsigned int N, const double sigma2 = 1)
{
  arma::vec grw(N);
  double sigma = sqrt(sigma2);
  for(unsigned int i = 0; i < N; i++){
      grw(i) = R::rnorm(0.0, sigma);
  }
  return cumsum(grw);
}


//' Generate an Moving Average Order 1 (MA(1)) Process
//' 
//' Generate an MA(1) Process given \eqn{\theta} and \eqn{\sigma^2}.
//' @param N      An \code{integer} for signal length.
//' @param theta  A \code{double} that contains moving average.
//' @param sigma2 A \code{double} that contains process variance.
//' @return A \code{vec} containing the MA(1) process.
//' @details
//' The function implements a way to generate the \eqn{x_t}{x[t]} values without calling the general ARMA function.
//' The moving average process is defined as \eqn{{x_t} = {w_t} + {\theta _1}{w_{t - 1}}}{x[t] = w[t] + theta*w[t-1]},
//'  where \eqn{{w_t}\mathop  \sim \limits^{iid} N\left( {0,\sigma _w^2} \right)}{w[t] ~ N(0,sigma^2) iid}
//' 
//' The function first generates a vector of white noise using \code{\link[gmwm]{gen_wn}} and then obtains the
//' MA values under the above equation.
//' 
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_ma1(10, .2, 1.2)
// [[Rcpp::export]]
arma::vec gen_ma1(const unsigned int N, const double theta = .3, const double sigma2 = 1)
{
  
  arma::vec wn = gen_wn(N+1, sigma2);
  arma::vec ma = arma::zeros<arma::vec>(N+1);
  for(unsigned int i=1; i <= N; i++ )
  {		
    ma(i) = theta*wn(i-1) + wn(i);
  }
  
  return ma.rows(1,N);
}

//' Generate an ARMA(1,1) sequence
//' 
//' Generate an ARMA(1,1) sequence given \eqn{\phi}, \eqn{\theta}, and \eqn{\sigma^2}.
//' @param N      An \code{integer} for signal length.
//' @param phi    A \code{double} that contains autoregressive.
//' @param theta  A \code{double} that contains moving average.
//' @param sigma2 A \code{double} that contains process variance.
//' @return A \code{vec} containing the MA(1) process.
//' @details
//' The function implements a way to generate the \eqn{x_t}{x[t]} values without calling the general ARMA function.
//' The autoregressive order 1 and moving average order 1 process is defined as \eqn{{x_t} = {\phi _1}{x_{t - 1}} + {w_t} + {\theta _1}{w_{t - 1}} }{x[t] = phi*x[t-1] + w[t] + theta*w[t-1]},
//'  where \eqn{{w_t}\mathop  \sim \limits^{iid} N\left( {0,\sigma _w^2} \right)}{w[t] ~ N(0,sigma^2) iid}
//' 
//' The function first generates a vector of white noise using \code{\link[gmwm]{gen_wn}} and then obtains the
//' ARMA values under the above equation.
//' 
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_ma1(10, .2, 1.2)
// [[Rcpp::export]]
arma::vec gen_arma11(const unsigned int N, const double phi = .1, const double theta = .3, const double sigma2 = 1)
{
  
  arma::vec wn = gen_wn(N+1, sigma2);
  arma::vec arma = arma::zeros<arma::vec>(N+1);
  for(unsigned int i=1; i <= N; i++ )
  {		
    arma(i) = phi*arma(i-1) + theta*wn(i-1) + wn(i);
  }
  
  return arma.rows(1,N);
}

//' Generate Autoregressive Order P - Moving Average Order Q (ARMA(P,Q)) Model
//' 
//' Generate an ARMA(P,Q) process with supplied vector of Autoregressive Coefficients (\eqn{\phi}), Moving Average Coefficients (\eqn{\theta}), and \eqn{\sigma^2}.
//' @param N       An \code{integer} for signal length.
//' @param ar      A \code{vec} that contains the AR coefficients.
//' @param ma      A \code{vec} that contains the MA coefficients.
//' @param sigma2  A \code{double} that contains process variance.
//' @param n_start An \code{unsigned int} that indicates the amount of observations to be used for the burn in period. 
//' @return A \code{vec} that contains the generated observations.
//' @details 
//' The innovations are generated from a normal distribution.
//' The \eqn{\sigma^2} parameter is indeed a variance parameter. 
//' This differs from R's use of the standard deviation, \eqn{\sigma}.
//' 
//' For AR(1), MA(1), and ARMA(1,1) please use their functions if speed is important.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' gen_arma(10, c(.3,.5), c(.1), 1, 0)
// [[Rcpp::export]]
arma::vec gen_arma(const unsigned int N,
                   const arma::vec& ar, const arma::vec& ma,
                   const double sigma2 = 1.5, 
                   unsigned int n_start = 0){
  
  // P = AR1 coefs, Q = MA coefs
  unsigned int p = ar.n_elem, q = ma.n_elem;

  // Need to append 1 to vectors.
  arma::vec one = arma::ones<arma::vec>(1);
  
  // What is the minimum root?
  double min_root = 1;
  
  // SD
  double sd = sqrt(sigma2);
  
  // Innovation save
  arma::vec innov(N);
  
  // Start Innovation
  arma::vec start_innov;
  
  // Store data
  arma::vec x;
  
  // Loop counter
  unsigned int i;
  
  // AR terms present? 
  if(p != 0){
    
    // Obtain the smallest root of the AR coefs.
    min_root = minroot(arma::conv_to<arma::cx_vec>::from(
                                      arma::join_cols(one, -ar)
                                    )
        
                      );
    
    // Check to see if the smallest root is not invertible (e.g. in unit circle)
    if(min_root <= 1){
      throw std::runtime_error("Supplied model's AR component is NOT invertible!");
    }
  }
  
  
  // Determine starting values
  if(n_start == 0){
    n_start = p + q + ( p > 0 ? ceil(6/log(min_root)) : 0 );
  }
  
  if(n_start < p + q){
    throw std::runtime_error("burn-in 'n.start' must be as long as 'ar + ma'");
  }
  
  // Generate Innovations
  for(i = 0; i < N; i++){
    innov(i) = R::rnorm(0,sd);
  }
  
  // Generate Starting Innovations
  start_innov = arma::vec(n_start);
  
  for(i = 0; i < n_start; i++){
    start_innov(i) = R::rnorm(0,sd);
  }
  
  // Combine
  x = join_cols(start_innov, innov);
  
  // Handle the MA part of ARMA
  if(q > 0){
    // Apply a convolution filter 
    // data, filter, sides, circular
    x = cfilter(x, join_cols(one,ma), 1, false);
    x.rows(0, q-1).fill(0);
  }
  
  // Handle the AR part of ARMA
  if(p > 0){
    // Apply recursive filter
    // data, filter, init value 
    // see comment in rfilter docs for init values (different than normal R)
    x = rfilter(x, ar, arma::zeros<arma::vec>(p));
  }
  
  // Remove starting innovations
  if(n_start > 0){
    // need -1 for C++ oob error 
    x = x.rows(n_start, x.n_elem - 1);
  }
  
  return x;
}

//' Generate Time Series based on Model (Internal)
//' 
//' Create a time series process based on a supplied \code{ts.model}.
//' @param N       An \code{interger} containing the amount of observations for the time series.
//' @param theta   A \code{vec} containing the parameters to use to generate the model
//' @param desc    A \code{vector<string>} containing the different model types (AR1, WN, etc..)
//' @param objdesc A \code{field<vec>} contains the different model objects e.g. AR1 = c(1,1)
//' @return A \code{vec} that contains combined time series.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' # AR
//' set.seed(1336)
//' gen_model(1000, c(.9,1), "AR1", list(c(1,1)))
// [[Rcpp::export]]
arma::vec gen_model(unsigned int N, const arma::vec& theta, const std::vector<std::string>& desc, const arma::field<arma::vec>& objdesc){
    arma::vec x  = arma::zeros<arma::vec>(N);
    unsigned int i_theta = 0;
    unsigned int num_desc = desc.size();
    
    for(unsigned int i = 0; i < num_desc; i++){
      // Need to add ARMA generation
      
      double theta_value = theta(i_theta);
      
      std::string element_type = desc[i];
      
  	  // AR 1
  	  if(element_type == "AR1" || element_type == "GM"){
  	    
  	    // First value is phi, increment for sigma2
  	    ++i_theta;
  	    
  	    // Get sigma2, this increment is taken care of at the end.
  	    double sig2 = theta(i_theta);
  	    
  	    // Compute theoretical WV
  	    x += gen_ar1(N, theta_value, sig2);
  	  } 
  	  // WN
  	  else if(element_type == "WN") {
  	    x += gen_wn(N, theta_value);
  	  } 
      // DR
  	  else if(element_type == "DR"){
  	    x += gen_dr(N, theta_value);
  	  }
      // QN
  	  else if(element_type == "QN"){
  	    x += gen_qn(N, theta_value);
  	  }
      // RW
  	  else if(element_type == "RW"){
  	    x += gen_rw(N, theta_value);
  	  } 
  	  // ARMA
  	  else {
  	    // Unpackage ARMA model parameter
  	    arma::vec model_params = objdesc(i);
  	    
  	    // Get position numbers (AR,MA,SIGMA2)
  	    unsigned int p = model_params(0);
  	    unsigned int q = model_params(1);
  	    
  	    // Set up temp storage
  	    arma::vec ar;
  	    arma::vec ma;
  	    
  	    // Get AR values
  	    if(p == 0){
  	      ar = arma::zeros<arma::vec>(0);
  	    }else{
  	      ar = theta.rows(i_theta,i_theta+p-1);
  	    }
  	    
  	    // Account for the number of P values
  	    i_theta += p;
  	    
  	    // Get MA values
  	    if(q == 0){
  	      ma = arma::zeros<arma::vec>(0); 
  	    }else{
  	      ma = theta.rows(i_theta,i_theta+q-1);
  	    }
  	    
  	    // Account for Q values
  	    i_theta += q;
  	    
  	    // Extract sigma2
  	    double sig2 = theta(i_theta);
  	    
  	    // Modified arima.sim
  	    x += gen_arma(N, ar, ma, sig2, 0);
  	  }
      
      // Increment theta once to account for popped value
      ++i_theta;
  }  
    
  return x;
}



//' Generate Latent Time Series based on Model (Internal)
//' 
//' Create a latent time series based on a supplied time series model.
//' @param N       An \code{interger} containing the amount of observations for the time series.
//' @param theta   A \code{vec} containing the parameters to use to generate the model.
//' @param desc    A \code{vector<string>} containing the different model types (AR1, WN, etc..).
//' @param objdesc A \code{field<vec>} containing the different model objects e.g. AR1 = c(1,1)
//' @return A \code{mat} containing data for each decomposed and combined time series.
//' @backref src/gen_process.cpp
//' @backref src/gen_process.h
//' @keywords internal
//' @examples
//' # AR
//' set.seed(1336)
//' gen_lts(10, c(.9,1), "AR1", list(c(1,1)))
// [[Rcpp::export]]
arma::mat gen_lts(unsigned int N, const arma::vec& theta, const std::vector<std::string>& desc, const arma::field<arma::vec>& objdesc){
  unsigned int i_theta = 0;
  unsigned int num_desc = desc.size();
  arma::mat x = arma::zeros<arma::mat>(N, num_desc+1);
  
  for(unsigned int i = 0; i < num_desc; i++){
    double theta_value = theta(i_theta);
    std::string element_type = desc[i];
    
    // AR 1
    if(element_type == "AR1" || element_type == "GM"){
      
      // First value is phi, increment for sigma2
      ++i_theta;
      
      // Get sigma2, this increment is taken care of at the end.
      double sig2 = theta(i_theta);
      
      // Compute theoretical WV
      // Store it into cube x
      x.col(i) = gen_ar1(N, theta_value, sig2);
      x.col(num_desc) += x.col(i);
      
    } 
    // WN
    else if(element_type == "WN") {
      x.col(i) = gen_wn(N, theta_value);
      x.col(num_desc) += x.col(i);
    }
    // DR
    else if(element_type == "DR"){
      x.col(i) = gen_dr(N, theta_value);
      x.col(num_desc) += x.col(i);

    }
    // QN
    else if(element_type == "QN"){
      x.col(i) = gen_qn(N, theta_value);
      x.col(num_desc) += x.col(i);
    }
    // RW
    else if(element_type == "RW"){
      x.col(i) = gen_rw(N, theta_value);
      x.col(num_desc) += x.col(i);
    }
    // ARMA
    else {
      // Unpackage ARMA model parameter
      arma::vec model_params = objdesc(i);
      
      // Get position numbers (AR,MA,SIGMA2)
      unsigned int p = model_params(0);
      unsigned int q = model_params(1);
      
      // Set up temp storage
      arma::vec ar;
      arma::vec ma;
      
      // Get AR values
      if(p == 0){
        ar = arma::zeros<arma::vec>(0);
      }else{
        ar = theta.rows(i_theta,i_theta+p-1);
      }
      
      // Account for the number of P values
      i_theta += p;
      
      // Get MA values
      if(q == 0){
        ma = arma::zeros<arma::vec>(0); 
      }else{
        ma = theta.rows(i_theta,i_theta+q-1);
      }
      
      // Account for Q values
      i_theta += q;
      
      // Extract sigma2
      double sig2 = theta(i_theta);
      
      // Modified arima.sim
      x.col(i) = gen_arma(N, ar, ma, sig2, 0);
      x.col(num_desc) += x.col(i);
    }
    
    // Increment theta once to account for popped value
    ++i_theta;
  }  
  
  return x;
}



/* --------------------- END Process Generation Functions -------------------------- */
