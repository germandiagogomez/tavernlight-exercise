
-- It removes, given a playerId, the party member called
-- membername.
function removePartyMember(playerId, membername)
  -- make player local, not a global
  local player = Player(playerId)
  local party = player:getParty()

  -- Take out of the loop Player(membername)
  -- to avoid redundant construction
  local wantedMemberId = Player(membername)

  -- Linear time, there are ways to do better but
  -- I am lacking more context here
  --
  -- I use more descriptive names for variables
  for _, aMemberId in pairs(party:getMembers()) do
    if aMemberId == wantedMemberId then
      party:removeMember(wantedMemberId)
      break -- break early when done, do not keep searching
    end
  end
end
