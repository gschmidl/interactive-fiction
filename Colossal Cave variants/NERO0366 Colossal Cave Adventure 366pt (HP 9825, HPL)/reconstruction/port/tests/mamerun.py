"""Play NERO0366 on the real program in MAME's hp9825t and capture what it prints.

usage: mamerun.py COMMANDS.txt OUT.txt [--width 80] [--disk DISK] [--restore]

--disk DISK  play on DISK (a copy of the disk is made there if it does not
             exist) and keep it, so a game suspended in one run can be
             restored in the next
--restore    answer "N" to "Do you want to start a new game?"

Boots the 9825T with the HP 9895 floppy (a scratch copy of ../../adventure.dsk),
loads and runs ADVENT, answers "Y" (new game), select code 6 and the width,
then types each line of COMMANDS.txt followed by CONTINUE, waiting for the
program to ask again before the next one.  The game's output goes to an HP
9871 printer on a 98032 GPIO card at select code 6 (MAME's switch value 5);
OUT.txt is what it printed, with CR LF turned into LF.

Needs MAME (E:\\CCA\\eXo\\emulators\\MAMEif, or $MAMEIF) with the hp9825t,
hp98034, hp9895 ROMs.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DISK = os.path.join(HERE, '..', '..', 'adventure.dsk')
MAME = os.environ.get('MAMEIF', r'E:\CCA\eXo\emulators\MAMEif')

CFG = '''<?xml version="1.0"?>
<mameconfig version="10">
    <system name="hp9825t">
        <input>
            <port tag=":slot0:98034_hpib:ieee_dev:hp9895:switches" type="CONFIG" mask="7" defvalue="0" value="7" />
            <port tag=":slot1:98032_gpio:SC" type="CONFIG" mask="15" defvalue="1" value="5" />
        </input>
    </system>
</mameconfig>
'''

LUA = r'''
local machine = manager.machine
local nk = machine.natkeyboard
local out = machine.output
local key0 = machine.ioport.ports[":KEY0"]
local keys = { execute = key0.fields["Execute"], continue = key0.fields["Continue"],
               run = key0.fields["Run"], stop = key0.fields["Stop"] }
local COMMANDS = %(commands)s
local WIDTH = "%(width)s"
local RESTORE = %(restore)s
local LOG = io.open(%(log)s, "w")
local function log(s) LOG:write(s .. "\n") LOG:flush() end

local function now() return machine.time:as_double() end
local function display()
  local rows = {}
  for d = 0, 31 do for r = 0, 6 do
    rows[#rows + 1] = string.char(out:get_value(string.format("char_%%d_%%d", d, r)) & 0x1f)
  end end
  return table.concat(rows)
end
local EMPTY = string.rep("\0", 7 * 32)
local CURSOR = "\1\1\1\31\1\1\1" .. string.rep("\0", 7 * 31)
local function busy() return out:get_value("run_light") ~= 0 end
local function ready() return not busy() and display() == CURSOR end
local before
local function changed()
  local d = display()
  return not busy() and d ~= before and d ~= CURSOR and d ~= EMPTY
end
local function frame() coroutine.yield() end
local function sleep(s) local t = now() while now() - t < s do frame() end end
local function wait_for(cond, timeout)
  local start, held = now(), nil
  while true do
    if cond() then
      held = held or now()
      if now() - held >= 0.5 then return end
    else
      held = nil
    end
    if now() - start > (timeout or 600) then error("timed out", 0) end
    frame()
  end
end
local function enter(text) nk:post(text) while not nk.empty do frame() end end
local function press(name)
  before = display()
  keys[name]:set_value(1) sleep(0.15) keys[name]:clear_value()
end

local function run()
  sleep(15)
  wait_for(ready)
  enter("drive 0,707") press("execute") wait_for(ready)
  enter('get "ADVENT"') press("execute") wait_for(ready)
  press("run") wait_for(changed)
  log("question 1")
  if RESTORE then
    enter("N") press("continue") wait_for(changed)
  else
    enter("Y") press("continue") wait_for(changed)
    log("question 2")
    enter("6") press("continue") wait_for(changed)
    log("question 3")
    enter(WIDTH) press("continue") wait_for(changed)
  end
  log("game started")
  for i, c in ipairs(COMMANDS) do
    enter(c) press("continue") wait_for(changed)
    log("command " .. i .. ": " .. c)
  end
  sleep(3)
end

machine.video.throttled = false
local co = coroutine.create(run)
local sub
sub = emu.add_machine_frame_notifier(function()
  local ok, err = coroutine.resume(co)
  if ok and coroutine.status(co) ~= "dead" then return end
  sub:unsubscribe()
  log(ok and "done" or ("error: " .. tostring(err)))
  LOG:close()
  machine:exit()
end)
'''


def lua_str(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def main():
    cmds_path, out_path = sys.argv[1], sys.argv[2]
    width = sys.argv[sys.argv.index('--width') + 1] if '--width' in sys.argv else '80'
    keep_disk = os.path.abspath(sys.argv[sys.argv.index('--disk') + 1]) if '--disk' in sys.argv else None
    restore = '--restore' in sys.argv
    cmds = [ln.rstrip('\r\n') for ln in open(cmds_path, encoding='ascii')]
    cmds = [c for c in cmds if c and not c.startswith('#')]
    tmp = tempfile.mkdtemp(prefix='nero_mame_')
    try:
        os.makedirs(os.path.join(tmp, 'cfg'))
        open(os.path.join(tmp, 'cfg', 'hp9825t.cfg'), 'w').write(CFG)
        disk = os.path.join(tmp, 'adventure.dsk')
        if keep_disk:
            if not os.path.exists(keep_disk):
                shutil.copy(DISK, keep_disk)
            disk = keep_disk
        else:
            shutil.copy(DISK, disk)
        logp = os.path.join(tmp, 'drive.log')
        lua = LUA % {'commands': '{' + ', '.join(lua_str(c) for c in cmds) + '}', 'width': width,
                     'log': lua_str(logp.replace('\\', '/')), 'restore': 'true' if restore else 'false'}
        open(os.path.join(tmp, 'drive.lua'), 'w').write(lua)
        prn = os.path.join(tmp, 'out.prn')
        args = [os.path.join(MAME, 'mame.exe'), 'hp9825t',
                '-cfg_directory', os.path.join(tmp, 'cfg'), '-nvram_directory', os.path.join(tmp, 'nvram'),
                '-numscreens', '1', '-rom1', 'mass2',
                '-slot0', '98034_hpib', '-slot0:98034_hpib:ieee_dev', 'hp9895',
                '-flop1', disk,
                '-slot1', '98032_gpio', '-slot1:98032_gpio:gpio', 'hp9871', '-prin1', prn,
                '-autoboot_script', os.path.join(tmp, 'drive.lua'),
                '-video', 'none', '-sound', 'none', '-skip_gameinfo', '-nothrottle', '-window']
        subprocess.run(args, cwd=MAME, timeout=3600)
        print(open(logp).read().strip().splitlines()[-1] if os.path.exists(logp) else 'no log')
        data = open(prn, 'rb').read() if os.path.exists(prn) else b''
        text = data.replace(b'\r\n', b'\n').replace(b'\r', b'\n').decode('latin-1')
        with open(out_path, 'w', encoding='latin-1', newline='\n') as f:
            f.write(text)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


main()
