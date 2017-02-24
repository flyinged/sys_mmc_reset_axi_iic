library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
library unisim;
use unisim.vcomponents.all;

entity ibufgds_module is
    port (
        i_clk_p : in  std_logic;
        i_clk_n : in  std_logic;
        o_clk   : out std_logic);
end ibufgds_module;

architecture behavioral of ibufgds_module is

begin

    ibufgds_0 : ibufgds
        port map (
            i  => i_clk_p,
            ib => i_clk_n,
            o  => o_clk);

end behavioral;
