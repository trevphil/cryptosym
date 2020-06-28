clear;
clc;

num_vars = 16704;
N = 30008;
num_hash_input_bits = 64;
avg_edges_per_node = 8;
directory = 'sha256-16704-30008-64';

% num_vars = 320;
% N = 1000;
% num_hash_input_bits = 64;
% avg_edges_per_node = 6;
% directory = 'pseudo_hash-320-1000-64';

input_file = sprintf('%s/data.bits', directory);
output_file = sprintf('%s/graph.csv', directory);

warning('TODO: not sure if Laplacian or pure adjacency matrix should be used');
warning('TODO: graph pruning algorithm is questionable');

disp('Loading samples...');
fileID = fopen(input_file);
samples = fread(fileID, num_vars * N, '*ubit1');
samples = reshape(samples, [N, num_vars])';
samples = cast(samples, 'double');
fclose(fileID);

disp('Computing matrix square...');
counts11 = samples * samples';

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

disp('Adding case 00, 01, 10, and 11...');
result = (r00 + r01 + r10 + r11) / N;

disp('Removing self-connections in adjacency matrix...');
result = result - diag(diag(result));

% disp('Saving raw ihat matrix...');
% save('sha256-matlab.mat', 'result', '-v7.3');

disp('Removing connections between hash input bits...');
result(256+1:256+num_hash_input_bits, 256+1:256+num_hash_input_bits) = 0;

disp('Normalizing such that each row sums to 1.0...');
result = result ./ sum(result, 2);

disp('Calculating eig of the matrix...');
[V, D] = eig(result);

disp('Sorting the eigenvectors and values ascending...');
[D, I] = sort(diag(D));
V = V(:, I);

disp('Extracting highest-weighted eigenvector as centrality measure...');
centrality_measure = abs(V(:, end));
centrality_measure = centrality_measure / sum(centrality_measure);

disp('Calculating edges per node...');
total_edges = avg_edges_per_node * num_vars;
edges_per_node = round(total_edges * centrality_measure);

disp('Performing column-wise sort for each row...');
[~, indices] = sort(result, 2);

disp('Pruning graph...');
[~, ordering] = sort(edges_per_node);
for rv = ordering(:)'
    desired_edges = min(edges_per_node(rv), num_vars - 1);
    result(rv, indices(rv, 1:end - desired_edges)) = 0;
    result(rv, indices(rv, end - desired_edges + 1:end)) = 1;
    result(:, rv) = result(rv, :);
end

if ~isequal(result, result.')
    warning('Adjacency matrix is not symmetric, something is wrong!');
end

min_connections = min(sum(result, 2));
max_connections = max(sum(result, 2));
avg_connections = round(mean(sum(result, 2)));
fprintf('Max connections: %d, min connections: %d, avg: %d\n',...
        max_connections, min_connections, avg_connections);

disp('Saving data...');
writematrix(result, output_file);

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
