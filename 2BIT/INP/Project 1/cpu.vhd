-- cpu.vhd: Simple 8-bit CPU (BrainFuck interpreter)
-- Copyright (C) 2024 Brno University of Technology,
--                    Faculty of Information Technology
-- Author(s): Matus Fignar <xfignam00 AT stud.fit.vutbr.cz>
--
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

-- ----------------------------------------------------------------------------
--                        Entity declaration
-- ----------------------------------------------------------------------------
entity cpu is
 port (
   CLK   : in std_logic;  -- hodinovy signal
   RESET : in std_logic;  -- asynchronni reset procesoru
   EN    : in std_logic;  -- povoleni cinnosti procesoru
 
   -- synchronni pamet RAM
   DATA_ADDR  : out std_logic_vector(12 downto 0); -- adresa do pameti
   DATA_WDATA : out std_logic_vector(7 downto 0); -- mem[DATA_ADDR] <- DATA_WDATA pokud DATA_EN='1'
   DATA_RDATA : in std_logic_vector(7 downto 0);  -- DATA_RDATA <- ram[DATA_ADDR] pokud DATA_EN='1'
   DATA_RDWR  : out std_logic;                    -- cteni (1) / zapis (0)
   DATA_EN    : out std_logic;                    -- povoleni cinnosti
   
   -- vstupni port
   IN_DATA   : in std_logic_vector(7 downto 0);   -- IN_DATA <- stav klavesnice pokud IN_VLD='1' a IN_REQ='1'
   IN_VLD    : in std_logic;                      -- data platna
   IN_REQ    : out std_logic;                     -- pozadavek na vstup data
   
   -- vystupni port
   OUT_DATA : out  std_logic_vector(7 downto 0);  -- zapisovana data
   OUT_BUSY : in std_logic;                       -- LCD je zaneprazdnen (1), nelze zapisovat
   OUT_INV  : out std_logic;                      -- pozadavek na aktivaci inverzniho zobrazeni (1)
   OUT_WE   : out std_logic;                      -- LCD <- OUT_DATA pokud OUT_WE='1' a OUT_BUSY='0'

   -- stavove signaly
   READY    : out std_logic;                      -- hodnota 1 znamena, ze byl procesor inicializovan a zacina vykonavat program
   DONE     : out std_logic                       -- hodnota 1 znamena, ze procesor ukoncil vykonavani programu (narazil na instrukci halt)
 );
end cpu;


-- ----------------------------------------------------------------------------
--                      Architecture declaration
-- ----------------------------------------------------------------------------
architecture behavioral of cpu is
-- PC
  signal pc_inc : std_logic;
  signal pc_dec : std_logic;
  signal pc_out : std_logic_vector(12 downto 0) := (others => '0');
-- PC

-- PTR
  signal ptr_inc : std_logic;
  signal ptr_dec : std_logic;
  signal ptr_out : std_logic_vector(12 downto 0);
-- PTR

-- CNT 
  signal cnt_inc : std_logic;
  signal cnt_dec : std_logic;
  signal cnt_out : std_logic_vector(7 downto 0);
-- CNT

-- TMP
  signal tmp_ld  : std_logic;
  signal tmp_out : std_logic_vector(7 downto 0);
-- TMP

-- MX
  signal sel1 : std_logic;
  signal sel2 : std_logic_vector(1 downto 0);
-- MX

--states
type fsm_state is (
  s_start,
  s_preinit,
  s_preinit1,
  s_init,

  s_get,
  s_decode,

  s_pointer_inc,
  s_pointer_dec,

  s_value_inc,
  s_value_dec,

  s_iteration_start,
  s_what_ins_when_0,
  s_test_for_end,

  s_iteration_end,
  s_test_for_begin,
  s_wait,

  s_load_tmp,
  s_get_tmp,

  s_print,
  s_load_in,

  s_halt
);
signal state  : fsm_state;
signal nstate : fsm_state; 
-- states


begin

-- process PTR
  ptr: process (CLK, RESET, ptr_inc, ptr_dec) is
    begin

      if (RESET = '1') then
        ptr_out <= (others => '0');

      elsif (rising_edge(CLK)) then
          if ptr_inc = '1' then
            if ptr_out = "1111111111111" then
              ptr_out  <= "0000000000000";
            else
              ptr_out <= ptr_out + 1;
            end if;
          elsif ptr_dec = '1' then
            if ptr_out = "0000000000000" then
              ptr_out  <= "1111111111111";
            else
              ptr_out <= ptr_out - 1;
              end if;
          end if;

      end if;
    end process;


 -- process PC
  pc : process(CLK, RESET, pc_dec, pc_inc)
    begin

      if RESET = '1' then
        pc_out <= (others => '0');
      elsif (rising_edge(CLK)) then
        if pc_inc = '1' then
          pc_out <= pc_out + 1;
        elsif pc_dec = '1' then
          pc_out <= pc_out - 1;
        end if;
      
      end if;
    end process;


 -- process TMP
  tmp : process(RESET, CLK,tmp_ld)
    begin

    if ( RESET = '1' ) then
      tmp_out <= (others => '0');

    elsif rising_edge(CLK) then
        if tmp_ld = '1' then
          tmp_out <= DATA_RDATA;
        end if;
    end if;

    end process;


 -- process CNT
  cnt : process (CLK, RESET, cnt_inc, cnt_dec) is
    begin

      if RESET = '1' then
        cnt_out <= (others => '0');
      elsif rising_edge(CLK) then
        if cnt_inc = '1' then
          cnt_out <= cnt_out + '1';
        elsif cnt_dec = '1' then
          cnt_out <= cnt_out - '1';
        end if;
      end if;

    end process;


 -- multiplexor 1
  mux1 : process (sel1, ptr_out, pc_out) is
    begin

        if sel1 = '0' then
          DATA_ADDR <= ptr_out;
        elsif sel1 = '1' then
          DATA_ADDR <= pc_out;
        else
        end if;
      
    end process;


 -- multiplexor 2
  mux2 : process(IN_DATA, tmp_out, sel2, DATA_RDATA) is
    begin
      
        case sel2 is
          when "00" => DATA_WDATA <= IN_DATA; 
          when "01" => DATA_WDATA <= tmp_out;
          when "10" => DATA_WDATA <= DATA_RDATA - 1;
          when "11" => DATA_WDATA <= DATA_RDATA + 1;
          when others => null;
        end case;

    end process;

--
  fsm_states_logic : process(CLK, RESET, EN) is
    begin
      if (RESET = '1') then
        state <= s_start;
      elsif rising_edge(CLK) and EN = '1' then
        state <= nstate;
      end if;
    end process;
  
  
  fsm : process (state, IN_VLD, IN_DATA, DATA_RDATA, OUT_BUSY, cnt_out) is
  begin
      --DATA_RDWR <= '0';
      
      DATA_EN <= '0';
      OUT_INV <= '0';
      IN_REQ  <= '0';
      OUT_WE  <= '0';
      READY   <= '0';
      DONE    <= '0';
      pc_inc  <= '0';
      pc_dec  <= '0';
      ptr_inc <= '0';
      ptr_dec  <= '0';
      sel1    <= '0';
      sel2    <= "00";
      tmp_ld <= '0';
      cnt_dec <= '0';
      cnt_inc <= '0';


      case state is
        when s_start =>
          READY <= '0';
          nstate <= s_preinit;
        
        when s_preinit =>
          sel1 <= '0';
          DATA_EN <= '1';
          DATA_RDWR <= '1';
          nstate <= s_preinit1;

        when s_preinit1 =>
          DATA_EN <= '1';
          DATA_RDWR <= '1';
          if DATA_RDATA = x"40" then
            nstate <= s_init;
          else
            ptr_inc <= '1';
            nstate <= s_preinit1;
          end if;
        
        when s_init =>
            READY <= '1';
            nstate <= s_get;

        when s_get =>
          sel1 <= '1';
          DATA_EN <= '1';
          DATA_RDWR <= '1';
          nstate <= s_decode;

        when s_decode => 
          case DATA_RDATA is
            when x"3E" => nstate <= s_pointer_inc;
            when x"3C" => nstate <= s_pointer_dec;
            when x"2B" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_value_inc;

            when x"2D" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_value_dec;

            when x"5B" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        pc_inc <= '1';
                        nstate <= s_iteration_start;

            when x"5D" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_iteration_end;

            when x"24" => 
                        sel1 <= '0';    
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_load_tmp; 

            when x"21" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_get_tmp;

            when x"2E" => 
                        sel1 <= '0';
                        DATA_EN <= '1';
                        DATA_RDWR <= '1';
                        nstate <= s_print;

            when x"2C" => nstate <= s_load_in;
            when x"40" => nstate <= s_halt;

            when others => 
                        pc_inc <= '1';
                        nstate <= s_get;
          end case;
        

        when s_pointer_inc =>
            ptr_inc <= '1';
            pc_inc <= '1';
            nstate <= s_get;

        when s_pointer_dec =>
            ptr_dec <= '1';
            pc_inc <= '1';
            nstate <= s_get;

        when s_value_inc =>
            DATA_EN <= '1';
            DATA_RDWR <= '0';
            sel2 <= "11";
            pc_inc <= '1';
            nstate <= s_get;

        when s_value_dec =>
            DATA_EN <= '1';
            DATA_RDWR <= '0';
            sel2 <= "10";
            pc_inc <= '1';
            nstate <= s_get;
        

        when s_iteration_start =>
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            if DATA_RDATA = "00000000" then
              cnt_inc <= '1';
              sel1 <= '1';
              pc_inc <= '1';
              nstate <= s_what_ins_when_0;
            else
              nstate <= s_get;
            end if;

        when s_what_ins_when_0 =>
            sel1 <= '1';
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            if cnt_out = "00000000" then
              
              nstate <= s_get;
            else
              if DATA_RDATA = x"5B" then
                cnt_inc <= '1';
                pc_inc <= '1';
                nstate <= s_what_ins_when_0;
              elsif DATA_RDATA = x"5D" then
                cnt_dec <= '1';
                if cnt_out = "00000001" then
                  nstate <= s_get;
                else
                  pc_inc <= '1';
                  nstate <= s_what_ins_when_0;
                end if;
              else
                pc_inc <= '1';
                nstate <= s_what_ins_when_0;
              end if;
            end if;

        when s_iteration_end =>
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            if DATA_RDATA = "00000000" then
              pc_inc <= '1';
              nstate <= s_get;
            else
              cnt_inc <= '1';
              pc_dec <= '1';
              sel1 <= '1';
              nstate <= s_wait;
            end if;
            
        when s_wait =>
            sel1 <= '1';
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            nstate <= s_test_for_begin;

        when s_test_for_begin =>
            sel1 <= '1';
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            if cnt_out = "00000000" then
              pc_inc <= '1';
              nstate <= s_get;
            else
              if DATA_RDATA = x"5D" then
                cnt_inc <= '1';
                pc_dec <= '1';
                nstate <= s_test_for_begin;
              elsif DATA_RDATA = x"5B" then
                cnt_dec <= '1';
                if cnt_out = "00000001" then
                  nstate <= s_test_for_begin;
                else 
                  pc_dec <= '1';
                  nstate <= s_test_for_begin;
                end if;
              else 
                pc_dec <= '1';
                nstate <= s_test_for_begin;
              end if ;
            end if;

        when s_load_in =>
            IN_REQ <= '1';
            if (IN_VLD = '0') then
              nstate <= s_load_in;
            else
              sel1 <= '0';
              DATA_EN <= '1';
              DATA_RDWR <= '0';
              sel2 <= "00";
              pc_inc <= '1';
              nstate <= s_get;
            end if;

        when s_load_tmp => 
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            tmp_ld <= '1';
            pc_inc <= '1';
            nstate <= s_get;

        when s_get_tmp =>
            DATA_EN <= '1';
            DATA_RDWR <= '0';
            sel2 <= "01";
            pc_inc <= '1';
            nstate <= s_get;
                        
        when s_print =>
            DATA_EN <= '1';
            DATA_RDWR <= '1';
            OUT_DATA <= DATA_RDATA;      
            if (OUT_BUSY = '1') then
              nstate <= s_print;
            else
              OUT_WE <= '1';
              pc_inc <= '1';
              nstate <= s_get;
            end if ;

        when s_halt =>
            DONE <= '1';
            READY <= '1';
            nstate <= s_halt;

        when others => null;
        
      end case;
  end process;
 
    -- pri tvorbe kodu reflektujte rady ze cviceni INP, zejmena mejte na pameti, ze 
 --   - nelze z vice procesu ovladat stejny signal,
 --   - je vhodne mit jeden proces pro popis jedne hardwarove komponenty, protoze pak
 --      - u synchronnich komponent obsahuje sensitivity list pouze CLK a RESET a 
 --      - u kombinacnich komponent obsahuje sensitivity list vsechny ctene signaly. 
end behavioral;

