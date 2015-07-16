-module(hilbert).
-include_lib("eunit/include/eunit.hrl").
-export([point2index/1]).
-export([run_nif_tests/0]).
-on_load(load_nif/0).

%%
%% API
%%


point2index(_P) ->
    erlang:nif_error(nif_not_loaded).

run_nif_tests() ->
    erlang:nif_error(nif_not_loaded).

load_nif() ->
    EbinDir = filename:dirname(code:which(?MODULE)),
    AppPath = filename:dirname(EbinDir),
    Path = filename:join([AppPath, "priv", "hilbert"]),
    ok = erlang:load_nif(Path, 0).

%%
%% TESTS
%%

nif_test() ->
    ?assert(hilbert:run_nif_tests()).

verification_test() ->
    ?assertEqual(<<36:32/little-integer>>,
                 point2index({<<1>>, <<2>>, <<3>>})),
    ?assertEqual(<<19622292:(32+32+32)/little-integer>>,
                 point2index({<<100:32/little-integer>>,
                              <<200:32/little-integer>>,
                              <<300:32/little-integer>>})),

    ?assertEqual(<<239,235,255,255,255,255,255,255,255,255,255,127>>,
                 point2index({<<9223372036854775807:64/little-integer>>,
                              <<100:32/little-integer>>})).


small_difference_test() ->
    A = {<<"somedim">>, <<"abc00">>},
    B = {<<"somedim">>, <<"abc01">>},
    P1 = point2index(A),
    P2 = point2index(B),
    BitSize = byte_size(P1)*8,
    <<I1:BitSize/little-integer>> = P1,
    <<I2:BitSize/little-integer>> = P2,
    ?assert(I1 < I2).


large_input_test() ->
    %% 32 bytes from crypto:rand_bytes(32)
    A = <<131,79,116,241,24,250,123,34,112,95,54,26,50,86,203,183,
          216,68,33,4,87,17,15,84,62,112,250,250,122,88,254,29>>,
    B = <<14,159,157,150,175,255,205,213,73,247,42,23,28,60,177,
          15,173,253,236,8,172,146,143,248,146,104,243,248,97,23,46,213>>,
    C = <<220,108,149,190,247,228,16,85,62,93,2,122,84,194,191,116,
          41,4,94,227,255,146, 124,123,33,66,34,181,24,85,33,127>>,

    Expected = <<55,84,207,102,246,34,98,187,223,163,31,183,50,65,156,37,168,
                 37,218,236,225,142,129,32,201,32,91,227,253,118,72,122,1,127,
                 240,5,24,187,29,160,175,98,67, 32,247,192,3,180,240,59,14,113,
                 151,43,47,125,35,105,174,61,192,33,203,49,121,147,112,182,108,
                 123,80,254,193,103,249,26,50,71,15,174,147,79,180,224,137,104,
                 192,158,94,208,5,208,103,127,253,120>>,
    ?assertEqual(byte_size(Expected), byte_size(A) + byte_size(B) + byte_size(C)),
    ?assertEqual(Expected, point2index({A, B, C})).
