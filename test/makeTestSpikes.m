% makeTestSpikes(1000,0.1,1) 

function makeTestSpikes(width, freq, maxTime)

spikeTimes = sort(maxTime*rand(ceil(width*freq*maxTime),1));

id = ceil(width*rand(size(spikeTimes)));

fid = fopen('spikes0.dat','w');
for i=1:length(spikeTimes)
    fprintf(fid,'%d %d\n', spikeTimes(i), id(i));
end

fclose(fid);

