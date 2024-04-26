// I will remove the memory leak but I will not limit myself to it.
//
// I assume that I can refactor code for a nicer API. In other circumstances
// I would probably just focus on other matters but since this is
// an exercise I think it can show my awareness of some other flaws
// in the function
//
// I assume this function runs in an interactive environment of some kind
// where the function executes in some authoritative backend server.
//
// Blocking could be considered something potentially bad in an
// environment like this.
//
// This has implications on the way I would design and correct
// the function if I had to design an API like this.
//

// since I see many things that can be improved, such as clearer
// ownership, better parameter passing and considering making
// the function asynchronous.
//
// To make ownership clearer, I will use smart pointers.
//
// The member function has at least two points where IO is performed.
// I assume IO is potentially expensive and it should be dealt with in
// some asynchronous way to avoid blocking the full thread.
//
// There are plenty of options:

//       - callback-based code (difficult to maintain but usually fast)
//       - future-style continuations
//       - lightweight threads such as Boost.Fiber
//       - Offloading IO to dedicated IO thread and return some kind of
//         handle that can be queried/scheduled (effectively making things async
//         actually depending on whether we need to wait or not for results)
//       - Use stackful coroutines such as in Boost.Coroutine
//       - Use "stackless" coroutines such as the ones found in C++20.
//
// and probably more.
//
// All have their own set of advantages and disadvantages.
//
// I am aware that for addItemToPlayer to work asynchronously I need some
// kind of scheduling system, which I won't show here (though I implemented one
// fully and another partially for C++20 coros in the past)
//
// Things I improved:
//
//    1. remove mutable global variable g_game, this adds invisible
//    dependencies.
//       You could either inject the game context in the constructor for Game
//       or use it as a parameter for the function, but do not make it an
//       implicit dependency.
//
//    2. In a fully synchronous function (not my solution) I would prefer
//       std::string_view over std::string const &, since
//       `game.addItemToPlayer(ctx, "myRecipient", itemId);` would make a
//       temporary std::string construction each time, while std::string_view is
//       flexible enough to allow both char array and strings with
//       never-allocating construction.
//    3. IO is done within the method, which is potentially expensive.
//
// This is my new implementation,

// Using flags and overloading operator| to avoid misuses of meaningless
// combinations
enum class Flag : unsigned char { NoLimit = 1, ... };

constexpr Flag operator|(Flag lhs, Flag rhs) {
  // C++23 std::to_underlying
  // could be used but I will stick to older version with a type trait
  using Underlying = std::underlying_type_t<Flag>;
  return Flag{static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs)};
}

constexpr Flag operator|=(Flag lhs, Flag rhs) {
  lhs = lhs | rhs;
  return lhs;
}

// Assume index is a size_t as traditionally in C++ and not an int,
// though that could be further discussed. Use an optional to expressively
// represent the absence of value. I assume the size of the parameter
// (an std::optional could be potentially bigger than a plain native type)
// is not a problem.
constexpr std::optional<std::size_t> INDEX_WHEREVER = std::nullopt;

// Make the function asynchronous, so it returns a Task<void> similar to
// libraries found around like https://github.com/lewissbaker/cppcoro
//
// Note this decision has been taken without much context, this is not
// probably what I would do myself, depending on having a better informed
// decision.
//
// If coroutines are too expensive or allocate memory further work would need
// to be done.

// @brief adds an item identified by `itemId` to a player with `recipient` into
// the game state
//
// @returns a Task which you can co_await
Task<void> Game::addItemToPlayer(GameContext &gameCtx, std::string recipient,
                                 std::uint16_t itemId) {
  // Assume unique_ptr and make ownership clear and RAII-friendly,
  // which will release. Also, pointers with custom deleters can be used,
  // which could be useful for custom allocation inside the if branch instead
  // of using plain global new, which could be a concern
  auto player = std::make_unique<Player>(gameCtx.getPlayerByName(recipient));
  if (!player) {
    // reset unique_pt
    player.reset(new Player(nullptr));
    // Potentially blocking IO: use co_await so that
    // the scheduler has a chance to do something else and release
    // the thread for other work
    if (!co_await IOLoginData::loadPlayerByName(player, recipient)) {
      co_return;
    }
  }

  // I assume the returned CreateItem returns memory to be released by us
  auto item = std::make_unique<Item>(Item::CreateItem(itemId));
  if (!item)
    co_return;

  // item.release() will release ownership of the item, which is passed to
  // internalAddItem FLAG_NOLIMIT replaced by Flag::NoLimit, a proper flag enum
  // that cannot be misused by accident. INDEX_WHEREVER is a named value which
  // is an empty optional. Clearer, no magic numbers, essentially free and
  // inlined except probably for the size of the optional.
  gameContext.internalAddItem(player->getInbox(), item.release(),
                              INDEX_WHEREEVER, Flag::NoLimit);

  if (player->isOffline()) {
    // Save: potentially blocking, so co_await so that the scheduler
    // can proceed with other work
    co_await IOLoginData::savePlayer(player);
  }
  co_return;
}
