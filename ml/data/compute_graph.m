close all;
clear;
clc;

should_plot = 1; % str2num(getenv('MATLAB_PLOT'));
edges_per_node = 6; % str2num(getenv('EDGES_PER_NODE'));
fprintf('edges per node: %d\n', edges_per_node);

if should_plot
    tiledlayout(1, 3);
    figure('Renderer', 'painters', 'Position', [10 10 1200 400])
end

%% ihat computation
% num_vars = 16704;
% N = 30008;
% num_hash_input_bits = 64;
% avg_edges_per_node = 8;
% directory = 'sha256-16704-30008-64';

num_vars = 320;
N = 1000;
num_hash_input_bits = 64;
directory = 'conditioned_on_input_and_hash-320-1000-64';

input_file = sprintf('%s/data.bits', directory);
output_file = sprintf('%s/graph.csv', directory);

warning('TODO: not sure if Laplacian or pure adjacency matrix should be used');

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

counts01 = N - counts11 - i0;
counts10 = N - counts11 - j0;
counts00 = N - counts11 - counts01 - counts10;

if ~isequal(counts00, counts00.')
    warning('counts00 is not symmetric!');
    return;
end

tmp = counts01 + counts10;
if ~isequal(tmp, tmp.')
    warning('counts01 + counts10 is not symmetric!');
    return;
end

if ~isequal(counts11, counts11.')
    warning('counts11 is not symmetric!');
    return;
end

disp('Computing ihats...');

disp('Computing case 00');
r00 = compute_ihat(counts00, i0, j0, N);
r00(isnan(r00)) = 0;
r00(isinf(r00)) = 0;
if ~isequal(r00, r00.')
    warning('r00 is not symmetric!');
    return;
end

disp('Computing case 01');
r01 = compute_ihat(counts01, i0, j1, N);
r01(isnan(r01)) = 0;
r01(isinf(r01)) = 0;

disp('Computing case 10');
r10 = compute_ihat(counts10, i1, j0, N);
r10(isnan(r10)) = 0;
r10(isinf(r10)) = 0;

tmp = r01 + r10;
if ~isequal(tmp, tmp.')
    warning('r01 + r10 is not symmetric!');
    return;
end

disp('Computing case 11');
r11 = compute_ihat(counts11, i1, j1, N);
r11(isnan(r11)) = 0;
r11(isinf(r11)) = 0;
if ~isequal(r11, r11.')
    warning('r11 is not symmetric!');
    return;
end

disp('Adding case 00, 01, 10, and 11...');
result = r00 + r01 + r10 + r11;

disp('Removing self-connections in adjacency matrix...');
result = result - diag(diag(result));

% disp('Saving raw ihat matrix...');
% save('sha256-matlab.mat', 'result', '-v7.3');

disp('Removing connections between hash input bits...');
result(256+1:256+num_hash_input_bits, 256+1:256+num_hash_input_bits) = 0;

if max(max(abs(result - result.'))) > 1e-14
    warning('ihat matrix is not symmetric!');
    return;
end

if sum(result(:) < 0) > 0
    warning('ihat should be positive-only!');
    return;
end

%% Plotting
if should_plot
    nexttile
    histogram(result(:), 50, 'Normalization', 'probability');

    nexttile
    imagesc(log(result))
    colormap('winter')
    colorbar
end
 
%% Graph pruning
disp('Pruning graph...');

adjacency_mat = result;
thresh = prctile(adjacency_mat(:), 80);
adjacency_mat(adjacency_mat < thresh) = 0;
adjacency_mat(adjacency_mat > 0) = 1;

prune = 1;
while prune
    prune = 0;
    for rv = 1:num_vars
        deg = sum(adjacency_mat, 1);
        if deg(rv) <= edges_per_node
            continue
        end

        indices_of_nonzero_nbrs = find(adjacency_mat(rv, :) > 0);
        ihats_of_nonzero_nbrs = result(rv, indices_of_nonzero_nbrs);
        [~, ascending_indices] = sort(ihats_of_nonzero_nbrs);
        for i = ascending_indices
            nbr = indices_of_nonzero_nbrs(i);
            if deg(nbr) > 1
                adjacency_mat(rv, nbr) = 0;
                adjacency_mat(nbr, rv) = 0;
                prune = 1;
                break;
            end
        end
    end
end

%% Calculate statistics

if should_plot
    nexttile
    colormap('hot');
    imagesc(adjacency_mat);

    figure;
    plot(graph(adjacency_mat), 'NodeColor', 'r');
end

min_connections = min(sum(adjacency_mat, 2));
max_connections = max(sum(adjacency_mat, 2));
avg_connections = round(mean(sum(adjacency_mat, 2)));
fprintf('Max connections: %d, min connections: %d, avg: %d\n',...
        max_connections, min_connections, avg_connections);

fprintf('Saving data to %s\n', output_file);
writematrix(adjacency_mat, output_file);

disp('Done.');

%% Helper function

function ihat = compute_ihat(counts, i, j, N)
    % Mutual information score between two RVs i and j is:
    %   score(i, j) = sum over all assignments of i=X and j=Y
    %                 of P(i=X, j=Y) * log[P(i=X, j=Y) / P(i=X)P(j=Y)]
    joint_prob = counts / N;
    ihat = joint_prob .* log(joint_prob ./ ( (i/N) .* (j/N) ));
end
