v(-2) refactor

looking at the GameState g

There are some visual effects in there that shouldn't be there. Or there might be a better way to do it.

Update -> Draw is the correct approach.

but right now we draw in update in some places. that has to go.

update the game state

read game state to know what pixels to draw

key thing about the game state is that it does not require player input to update.

we should think about using events to trigger draws

events of the same type should be executed in parallel



