input_file = 'sha256-30000.csv';
output_file = 'sha256-30000-ihat.csv';

disp('Loading samples...');
samples = readmatrix(input_file);
[N, num_vars] = size(samples);

disp('Computing matrix square...');
counts11 = samples' * samples;

disp('Deriving 00, 01, and 10 cases from 11 case...');
diag1 = diag(counts11);
diag0 = N - diag1;

i1 = repmat(diag1', num_vars, 1);
j1 = i1';

i0 = repmat(diag0', num_vars, 1);
j0 = i0';

counts01 = N - counts11 - j0;
counts10 = N - counts11 - i0;
counts00 = N - counts11 - counts01 - counts10;

disp('Computing ihats...');

C = log(N) * ones(size(counts11));

disp('Computing case 00');
r00 = compute_ihat(counts00, i0, j0, C);
r00(isnan(r00)) = 0;
r00(isinf(r00)) = 0;

disp('Computing case 01');
r01 = compute_ihat(counts01, i0, j1, C);
r01(isnan(r01)) = 0;
r01(isinf(r01)) = 0;

disp('Computing case 10');
r10 = compute_ihat(counts10, i1, j0, C);
r10(isnan(r10)) = 0;
r10(isinf(r10)) = 0;

disp('Computing case 11');
r11 = compute_ihat(counts11, i1, j1, C);
r11(isnan(r11)) = 0;
r11(isinf(r11)) = 0;

output = (r00 + r01 + r10 + r11) / N;

disp('Removing self-connections in adjacency matrix...');
output = output - diag(diag(output));

writematrix(output, output_file);
disp('Done.');

function ihat = compute_ihat(counts, i, j, C)
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
    %
    % The division by N is done at the very end (not in this function).

    ihat = counts .* (C + log(counts) - log(i) - log(j));
end