-- Output a low-active reset for a while after power-up.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.std_logic_misc.all;
use ieee.std_logic_unsigned.all;
library unisim;
use unisim.vcomponents.all;

entity module_ext_reset is
    port (
        reset_in_b  : in  std_logic;
        clk         : in  std_logic;
        reset_out_b : out std_logic);
end module_ext_reset;

architecture behavioral of module_ext_reset is

    signal reset_in_sync1 : std_logic                     := '0';
    signal reset_in_sync2 : std_logic                     := '0';
    signal cnt            : std_logic_vector(23 downto 0) := (others => '0');

begin

    reset_in_sync1 <= reset_in_b     when rising_edge(clk);
    reset_in_sync2 <= reset_in_sync1 when rising_edge(clk);

    process (clk)
    begin
        if rising_edge(clk) then
            if reset_in_sync2 = '0' then
                cnt <= (others => '0');
            else
                cnt <= cnt + not(and_reduce(cnt));
            end if;
        end if;
    end process;

    reset_out_b <= and_reduce(cnt) when rising_edge(clk);

end behavioral;
