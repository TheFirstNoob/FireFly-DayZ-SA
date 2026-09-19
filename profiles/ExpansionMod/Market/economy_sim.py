# -*- coding: utf-8 -*-
"""
ЭМУЛЯТОР ЭКОНОМИКИ FireFly DayZ (v4 — финальный инструмент)

ТОЧКА ОПОРЫ: вся экономика выводится из одной цифры —
  «сколько часов соло-игры до покупки машины (цели)».

ФОРМУЛА:
  1. Якорь: целевой чистый доход/час = Цена_цели / Часы_до_цели
  2. Валовый доход/час = Чистый / (1 - доля_расходов)
  3. MinPrice тира подбирается так, чтобы честная выручка за день
     (с деградацией цены от стока, sell% и condition) = целевой вклад тира
  4. MaxPrice = MinPrice * 5  (соотношение 1:5 / "80-20")
  5. MaxStock — рычаг скорости падения цены (разделяет тиры)

Режимы:
  python economy_sim.py calibrate   — целевые Min/Max по тирам
  python economy_sim.py simulate    — состояние текущих market-файлов
  python economy_sim.py verify      — целевые цены применены, проверка
  python economy_sim.py scenario    — сравнение sell 20% vs 75%

Настройки — в CONFIG ниже.
"""
import json, os, random, sys
from collections import defaultdict

CONFIG = {
    "players": 25,
    "hours_per_day": 6,
    "sim_days": 30,
    "seed": 42,

    # --- ЯКОРЬ (точка опоры) ---
    "target_price": 100000,      # цена машины
    "target_hours": 15,          # часов соло-игры до машины
    "cost_share": 0.40,          # доля дохода на расходы (патроны/еда/ремонт)

    # --- ЛУТ ---
    "loot_per_hour": 5,          # предметов в час (находит и продаёт)
    "loot_tier_dist": {1: 0.50, 2: 0.30, 3: 0.15, 4: 0.04, 5: 0.01},
    "tier_cap": 5,               # какие тиры лутаются (T5 сейчас переделывается)

    # --- ТОРГОВЕЦ ---
    "market_dir": r"D:\GitHub\FireFly-DayZ-SA\profiles\ExpansionMod\Market",
    "zone_sell_pct": 20.0,       # 80/20: торговец платит 20% цены
    "zone_buy_pct": 100.0,
    "condition_pct": 0.75,       # среднее состояние лута (Worn)
    "trader_sell_tiers": [1, 2, 3, 4],   # торговец принимает (фонтан)
    "trader_buy_tiers": [1, 2],          # торговец продаёт (сток)

    # --- P2P АУКЦИОН ---
    "auction_listing_pct": 30.0, # комиссия при выставлении (сток)
    "auction_owner_discount_pct": 70.0,
    "auction_markup": 1.6,       # наценка лота над ценой торговца
    "p2p_share": {1: 0.0, 2: 0.10, 3: 0.40, 4: 0.70, 5: 0.90},

    # --- РАСХОДЫ (сток) ---
    "daily_cost_base": 1500,
    "death_prob_per_day": 0.15,
    "death_loss_pct": 0.05,
}

# типовой MaxStock по тирам (медианы из реальных файлов сервера)
TYPICAL_STOCK = {1: 100, 2: 100, 3: 60, 4: 25, 5: 5}
TIER_NAMES = {1: "мусор", 2: "обычное", 3: "редкое", 4: "эпик", 5: "легенда"}

def clamp(v, a=0, b=1):
    return max(a, min(b, v))

def power_conversion(min_from, max_from, value, min_to, max_to, power=6.0):
    if max_from == min_from:
        return min_to
    v = clamp((value - min_from) / (max_from - min_from))
    return (1 - (1 - v) ** power) * (max_to - min_to) + min_to

class MarketItem:
    def __init__(self, cls, min_p, max_p, min_stock, max_stock):
        self.cls = cls
        self.min_p = min_p
        self.max_p = max_p
        self.min_stock = min_stock
        self.max_stock = max_stock
        self.stock = 0

    def is_static(self):
        return self.max_stock > 0 and self.min_stock == self.max_stock

    def calc_price(self, stock):
        if self.is_static() or self.max_stock == 0:
            return self.min_p
        return power_conversion(self.min_stock, self.max_stock, stock,
                                self.max_p, self.min_p, 6.0)

    def price_sell(self, condition=1.0):
        p = self.calc_price(self.stock) * (CONFIG["zone_sell_pct"] / 100.0) * condition
        self.stock += 1
        return p

    def tier(self):
        m = self.min_p
        if m < 500: return 1
        if m < 3000: return 2
        if m < 15000: return 3
        if m < 60000: return 4
        return 5

def load_market(dir_path):
    items = []
    for fn in os.listdir(dir_path):
        if not fn.endswith(".json"):
            continue
        try:
            d = json.load(open(os.path.join(dir_path, fn), encoding="utf-8"))
        except Exception:
            continue
        init_pct = d.get("InitStockPercent", 25.0) / 100.0
        for it in d.get("Items", []):
            mi = MarketItem(it["ClassName"],
                            float(it.get("MinPriceThreshold", 0)),
                            float(it.get("MaxPriceThreshold", 0)),
                            float(it.get("MinStockThreshold", 0)),
                            float(it.get("MaxStockThreshold", 0)))
            if mi.max_stock > 0 and not mi.is_static():
                mi.stock = int(mi.max_stock * init_pct)
            else:
                mi.stock = mi.max_stock if mi.is_static() else 1
            items.append(mi)
    return items

def tier_buckets(items):
    buckets = defaultdict(list)
    for it in items:
        buckets[it.tier()].append(it)
    return buckets

def calc_target_min(cfg, tier, n_per_day, max_stock, init_pct):
    """Подбирает MinPrice так, чтобы честная выручка за день (с деградацией
    стока и sell%) равнялась целевому вкладу тира."""
    target_day = (cfg["target_price"] / cfg["target_hours"]
                  / (1 - cfg["cost_share"]) * cfg["hours_per_day"]
                  * cfg["loot_tier_dist"][tier])
    sell_eff = (cfg["zone_sell_pct"] / 100.0) * cfg["condition_pct"]

    def revenue(min_price):
        max_p = min_price * 5
        stock = int(max_stock * init_pct)
        total = 0.0
        for _ in range(int(n_per_day)):
            total += power_conversion(0, max_stock, stock, max_p, min_price, 6.0) * sell_eff
            stock += 1
        return total

    lo, hi = 1.0, 50_000_000.0
    for _ in range(70):
        mid = (lo + hi) / 2
        if revenue(mid) < target_day:
            lo = mid
        else:
            hi = mid
    return (lo + hi) / 2

def calibrate(cfg):
    dist = cfg["loot_tier_dist"]
    items_per_day = cfg["loot_per_hour"] * cfg["hours_per_day"]
    d_net = cfg["target_price"] / cfg["target_hours"]
    d_gross = d_net / (1 - cfg["cost_share"])
    sell_eff = (cfg["zone_sell_pct"] / 100.0) * cfg["condition_pct"]

    L = []
    A = L.append
    A("=" * 78)
    A("КАЛИБРАТОР — целевые ценники по тирам")
    A(f"цель: {cfg['target_price']:,} за {cfg['target_hours']} ч | чистый {d_net:,.0f}/ч | валовый {d_gross:,.0f}/ч")
    A(f"лут {cfg['loot_per_hour']}/ч ({items_per_day}/день) | sell {cfg['zone_sell_pct']}% x cond {cfg['condition_pct']} = {sell_eff:.0%}")
    A(f"расходы {cfg['cost_share']:.0%} | типовой MaxStock: {TYPICAL_STOCK}")
    A("=" * 78)
    A(f"{'Тир':<5}{'доля':>7}{'шт/день':>8}{'Min':>14}{'Max (x5)':>14}{'вклад в доход':>15}")
    results = {}
    for t in sorted(dist):
        prob = dist[t]
        n = items_per_day * prob
        min_p = calc_target_min(cfg, t, n, TYPICAL_STOCK[t], 0.25)
        results[t] = (min_p, min_p * 5)
        target_day = d_gross * cfg["hours_per_day"] * prob
        A(f"T{t} {TIER_NAMES[t]:<9}{prob:>6.0%}{n:>8.0f}{min_p:>12,.0f}{min_p*5:>14,.0f}{target_day:>15,.0f}")
    A("")
    A("ИТОГ — записывай в маркет-файлы (Min / Max):")
    for t in sorted(results):
        A(f"  T{t}: Min {results[t][0]:>10,.0f} | Max {results[t][1]:>12,.0f}")
    return "\n".join(L), results

class Player:
    def __init__(self, pid):
        self.pid = pid
        self.money = 0.0
        self.earned_trader = 0.0
        self.earned_auction = 0.0
        self.spent_trader = 0.0
        self.spent_auction = 0.0
        self.death_loss = 0.0

class Auction:
    def __init__(self, cfg):
        self.listing_pct = cfg["auction_listing_pct"] / 100.0
        self.owner_disc = cfg["auction_owner_discount_pct"] / 100.0
        self.markup = cfg["auction_markup"]
        self.active = []
        self.fee_sink = 0.0

    def list(self, trader_price, seller_idx, rng):
        listed = trader_price * self.markup
        fee = listed * self.listing_pct
        self.active.append((trader_price, seller_idx, listed))
        self.fee_sink += fee
        return fee

    def settle(self, players, rng):
        still = []
        for base, seller_idx, listed in self.active:
            prob = 0.25 + 0.03 * len(players)
            if rng.random() < prob:
                cands = [p for p in players if p.pid != seller_idx and p.money >= listed * 0.5]
                if cands:
                    buyer = rng.choice(cands)
                    buyer.money -= listed
                    buyer.spent_auction += listed
                    players[seller_idx].money += listed
                    players[seller_idx].earned_auction += listed
                else:
                    still.append((base, seller_idx, listed))
            else:
                back = listed * self.owner_disc
                players[seller_idx].money += back
                players[seller_idx].earned_auction += back
        self.active = still

def simulate(items, cfg, rng):
    buckets = tier_buckets(items)
    cap = cfg.get("tier_cap", 5)
    players = [Player(i) for i in range(cfg["players"])]
    auction = Auction(cfg)
    history = []
    for day in range(1, cfg["sim_days"] + 1):
        f_day = 0.0
        s_day = 0.0
        for pl in players:
            n_loot = int(cfg["loot_per_hour"] * cfg["hours_per_day"])
            for _ in range(n_loot):
                r = rng.random()
                acc = 0.0
                tier = 1
                for t, prob in sorted(cfg["loot_tier_dist"].items()):
                    acc += prob
                    if r <= acc:
                        tier = t
                        break
                if tier > cap:
                    continue
                bkt = buckets.get(tier)
                if not bkt:
                    continue
                it = rng.choice(bkt)
                cond = cfg["condition_pct"]
                if tier in cfg["trader_sell_tiers"]:
                    trader_price = it.price_sell(cond)
                    p2p_prob = cfg["p2p_share"].get(tier, 0.0)
                    if rng.random() < p2p_prob:
                        fee = auction.list(trader_price, pl.pid, rng)
                        pl.money -= fee
                        s_day += fee
                    else:
                        pl.money += trader_price
                        pl.earned_trader += trader_price
                        f_day += trader_price
                else:
                    if tier in cfg["p2p_share"] and cfg["p2p_share"][tier] > 0:
                        ref_price = it.min_p * (cfg["zone_sell_pct"]/100.0) * cfg["condition_pct"]
                        fee = auction.list(ref_price, pl.pid, rng)
                        pl.money -= fee
                        s_day += fee
            spend = min(pl.money, cfg["daily_cost_base"])
            pl.money -= spend
            pl.spent_trader += spend
            s_day += spend
            if cfg["death_prob_per_day"] > 0 and rng.random() < cfg["death_prob_per_day"]:
                loss = pl.money * cfg["death_loss_pct"]
                pl.money -= loss
                pl.death_loss += loss
                s_day += loss
        auction.settle(players, rng)
        history.append((day, f_day, s_day, sum(p.money for p in players)))
    return players, history, buckets, auction

def report_sim(players, history, buckets, cfg, auction, title="СИМУЛЯЦИЯ"):
    L = []
    A = L.append
    A("=" * 76)
    A(title)
    A(f"игроков={cfg['players']}  часов/день={cfg['hours_per_day']}  дней={cfg['sim_days']}")
    A(f"торговец: sell={cfg['zone_sell_pct']}% | принимает тиры {cfg['trader_sell_tiers']} | продаёт тиры {cfg['trader_buy_tiers']}")
    A(f"аукцион: комиссия={cfg['auction_listing_pct']}% наценка={cfg['auction_markup']}x | п2п {cfg['p2p_share']}")
    A("=" * 76)
    total_f = sum(f for d, f, s, m in history)
    total_s = sum(s for d, f, s, m in history)
    ratio = total_f / max(total_s, 1)
    A(f"фонтан (торговец): {total_f:>12,.0f} | сток: {total_s:>12,.0f} | баланс x{ratio:.2f} "
      + ("ИНФЛЯЦИЯ" if ratio > 1.3 else "ДЕФЛЯЦИЯ" if ratio < 0.7 else "ОК"))
    total_earn = sum(p.earned_trader + p.earned_auction for p in players)
    avg_day = total_earn / (cfg["players"] * cfg["sim_days"])
    tr = sum(p.earned_trader for p in players)
    au = sum(p.earned_auction for p in players)
    A(f"доход игрока: {avg_day:,.0f}/день | {avg_day/cfg['hours_per_day']:,.0f}/час | торговец {tr/max(total_earn,1)*100:.0f}% | аукцион {au/max(total_earn,1)*100:.0f}%")
    A(f"средний баланс: {sum(p.money for p in players)/len(players):,.0f} | комиссия аукциона сгорела: {auction.fee_sink:,.0f}")
    A("ВРЕМЯ ДО ЦЕЛИ:")
    for name, price in [("Пистолет", 3000), ("Штурмовка", 15000), ("SV98", 61250), ("Машина", 100000)]:
        days = price / avg_day if avg_day > 0 else float("inf")
        A(f"  {name:<12} {price:>8,.0f} -> ~{days:5.1f} дн ({days*cfg['hours_per_day']:4.0f} ч)")
    return "\n".join(L)

def main():
    sys.stdout.reconfigure(encoding="utf-8")
    cfg = dict(CONFIG)
    mode = sys.argv[1] if len(sys.argv) > 1 else "calibrate"

    if mode == "calibrate":
        txt, _ = calibrate(cfg)
        print(txt)

    elif mode == "simulate":
        items = load_market(cfg["market_dir"])
        rng = random.Random(cfg["seed"])
        players, history, buckets, auction = simulate(items, cfg, rng)
        print(report_sim(players, history, buckets, cfg, auction,
                         "СИМУЛЯЦИЯ — текущие market-файлы"))

    elif mode == "verify":
        items = load_market(cfg["market_dir"])
        txt, results = calibrate(cfg)
        print(txt)
        for it in items:
            t = it.tier()
            if t in results and not it.is_static():
                it.min_p, it.max_p = results[t]
        rng = random.Random(cfg["seed"])
        players, history, buckets, auction = simulate(items, cfg, rng)
        print()
        print(report_sim(players, history, buckets, cfg, auction,
                         "СИМУЛЯЦИЯ — с целевыми ценами из калибратора"))

    elif mode == "scenario":
        txt, _ = calibrate(cfg)
        print(txt)
        print()
        print("--- Сценарий: sell 75% (текущий дефолт Expansion) ---")
        cfg2 = dict(cfg)
        cfg2["zone_sell_pct"] = 75.0
        txt2, results2 = calibrate(cfg2)
        print(txt2)
        print()
        print("--- Сравнение Min по тирам (sell 20% vs 75%) ---")
        for t in sorted(results2):
            print(f"T{t}: sell20% Min={calibrate(dict(cfg))[1][t][0]:,.0f} | sell75% Min={results2[t][0]:,.0f}")

if __name__ == "__main__":
    main()