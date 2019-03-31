% created by: Dordije Boskovic

function [results,mapexcluded] = hyperAceR_RTA_SM_I(M, S, beta, th)

    if(nargin<3) 
        th = 0.1;
        beta = 10000;
    end
    
	[p, N] = size(M);
    t = round(N/100);
    
    G = beta*eye(p,p);
	
    res_mean = 0;
    mapexcluded = zeros(N,1);
    

    
    results = zeros(1, N);
  
    for k = 1:N

         x = M(:,k);

        %calculate detection statistic
        tmp2 = S.'*G;
        tmp = (tmp2*S);
        results(k) = (tmp2*x)^2 / (tmp*(x.'*G*x));

        
        %added to calculate mean and exclude estimated targets from mean
        if(k > t) 
            res_mean = 0;
            count = 0;
            for p = (k-t) : (k-1)
                if(mapexcluded(p) ~= 1) 
                 res_mean = res_mean + results(p);   
                 count = count + 1;
                end
                res_mean = res_mean / count;
            end
        end


        if(results(k) - res_mean >= th)
            mapexcluded (k) = 1;
           
            if(k < t)
                  G = G - ((G*x)*(x'*G))./(1+x'*G*x); 
            end 

        else
              G = G - ((G*x)*(x'*G))./(1+x'*G*x);
        end	
        
    end
    
    %% restream
    for k = 1:t

        x = M(:,k);

        %calculate detection statistic
        tmp2 = S.'*G;
        tmp = (tmp2*S);
        results(k) = (tmp2*x)^2 / (tmp*(x.'*G*x));

    end

end





