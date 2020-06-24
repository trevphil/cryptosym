disp('Loading samples...');
samples = readmatrix('sha256-30000.csv');

ones_mask = ones(size(samples));
inv_samples = ones_mask - samples;

disp('Computing case 11');
r11 = compute_ihat(samples, samples);
r11(isnan(r11)) = 0;
r11(isinf(r11)) = 0;

disp('Computing case 01');
r01 = compute_ihat(inv_samples, samples);
r01(isnan(r01)) = 0;
r01(isinf(r01)) = 0;

disp('Computing case 10');
r10 = compute_ihat(samples, inv_samples);
r10(isnan(r10)) = 0;
r10(isinf(r10)) = 0;

disp('Computing case 00');
r00 = compute_ihat(inv_samples, inv_samples);
r00(isnan(r00)) = 0;
r00(isinf(r00)) = 0;

result = r00 + r01 + r10 + r11;

writematrix(result, 'sha256-30000-ihat.csv');
disp('Done.');

function ihat = compute_ihat(A, B)
    % Mutual information score between two RVs i and j is:
    %   score(i, j) = sum over all assignments of i=X and j=Y
    %                 of P(i=X, j=Y) * log[P(i=X, j=Y) / P(i=X)P(j=Y)]
    %
    % Can be approximated: P(i=X) = Count(i=X) / N
    %                      P(i=X, j=Y) = Count(i=X, j=Y) / N
    %
    % Thus the score simplifies to:
    %   score(i, j) = Count(i=X, j=Y) / N * (log N
    %                                        + log(Count(i=X, j=Y))
    %                                        - log(Count(i=X)Count(j=Y)))
    N = 30000;

    disp('Squaring the matrix...');
    counts = A' * B;
    
    disp('Setting up the result...');
    diagonal = diag(counts);  % vector of diagonal elements in "counts"
    counts = triu(counts);  % upper triangular matrix
    counts = counts - diag(diagonal);  % zero the diagonal elements
    
    disp('Computing diagonal product...');
    denom = diagonal' * diagonal; % denom(i, j) = count(i) * count(j)

    disp('Computing constant...');
    C = log(N) * ones(size(counts)); % C = log(N) in the correct shape

    disp('Computing ihat...');
    ihat = counts .* (C + log(counts) - log(denom)) / N;
end