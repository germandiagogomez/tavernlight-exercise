-- To avoid mallicious input, such as SQL injection or subqueries
-- that make no sense and can leak data or endanger server safety,
-- I would restrict memberCount with parameter validation: enforce it is a positive
-- number in a given range
--
-- Also, the storage query is potentially blocking and the return could be several
-- guild names, so we need to iterate the result and make the API asynchronous.
-- Those must be corrected.
--
-- I will use a callback to avoid blocking
-- I just need to print and not return a result, so we can do that in this case
-- easily since we do not need to wait for any value to be returned

-- Probably setting a SQL limit would be also a good idea if the
-- printing becomes too heavy
local SMALL_GUILD_MAX = 100

local queries_in_progress = 0
local MAX_ALLOWED_CONCURRENCY = 30

-- Takes a mememberCount, which must be a number and returns the guilds that
-- have less members in the guild. To avoid saturation of async API call,
-- a limit is fixed. The function will return true when it could schedule
-- the query and false if it could not

-- Bonus: Implement flow control to not overload
function printSmallGuildNames(memberCount)
  queries_in_progress = queries_in_progress + 1
  if queries_in_progress > MAX_ALLOWED_CONCURRENCY then
    return false
  end
  -- Parameter validation
  if type(memberCount) ~= "number" or memberCount < 1 or memberCount > SMALL_GUILD_MAX then
    queries_in_progress = queries_in_progress - 1
    error("memberCount must be a number")
  end

  -- Execute asynchronously and callback will print
  local selectGuildQuery = "SELECT name FROM guilds WHERE max_members < %d;"
  local printResult = function (resulId)
                       if resultId then
                         repeat
                           local guildName = resultId.getDataString(resultId, "name")
                           print(guildName)
                         until not resultId.next(query)
                         queries_in_progress = queries_in_progress - 1
                       end
                     end
  -- This call will not wait, will just do its thing and when done
  -- printResult will be invoked.
  db.asyncStoreQuery(string.format(selectGuildQuery, memberCount), printResult)
  return true
end
