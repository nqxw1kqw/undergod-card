"""Author the campaign's JSON content; no image generation is performed here."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data'
def write(name, value):
    (DATA / name).write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
def effect(op, amount, target='self', **extra):
    return dict(op=op, amount=amount, target=target, **extra)
def damage(amount, target='enemy', **extra): return effect('damage', amount, target, **extra)
def status(name, amount, target='enemy'): return effect('apply', amount, target, status=name)

legacy = ['punch','shield','heavy','cookie','peck','double_peck','feather','millet','stomp','crow','wing','venom_beak','snare','feather_rain','guard_up','grub','storm_spear','star_tip','toxin']
old = {c['id']: c for c in json.loads((DATA/'cards.json').read_text(encoding='utf-8'))}
cards = [old[key] for key in legacy]
for card in cards:
    card['rarity'] = 'rare' if card['rarity']=='uncommon' else card['rarity']
    card['type'] = 'attack' if card.get('damage') or any(e['op']=='damage' for e in card.get('effects',[])) else 'skill'
    card['ranged'] = card['id'] in ('punch','heavy','storm_spear','star_tip','feather_rain')
cards[3]['exhaust']=True;cards[3]['description']='Hồi 8 HP. Tiêu hao.'
old['double_peck'].update(name='Mổ Liên Hoàn',effects=[damage(3,times=3)],description='Gây 3 sát thương ×3.')
old['crow'].update(name='Gáy Sáng',effects=[effect('temp_strength',2)],description='Nhận 2 Sức Mạnh trong lượt này.')
old['venom_beak'].update(name='Cú Mổ Độc',effects=[damage(5),status('poison',3)],description='Gây 5 sát thương và 3 Độc.')
# The objects in cards came from old, so these edits are shared.
def card(key,name,cost,description,effects,rarity='common',**extra):
    cards.append(dict(id=key,name=name,cost=cost,description=description,effects=effects,rarity=rarity,copies=0,type='attack' if any(e['op']=='damage' for e in effects) else 'skill',**extra))
card('wingbeat','Vỗ Cánh',0,'Rút 1 lá, nhận 5 Nộ.',[effect('draw',1),effect('rage',5)])
card('feather_shield','Khiên Lông Vũ',1,'Nhận 4 giáp. Giữ giáp một lượt.',[effect('block',4),effect('retain',1)])
card('piercing_spear','Lao Xuyên',2,'Gây 10 sát thương xuyên giáp.',[damage(10,pierce=True)],ranged=True)
card('counter','Phản Đòn',1,'Nhận 6 giáp. Mỗi lần bị đánh trong lượt địch, phản 4.',[effect('block',6),effect('thorns',4)],'rare')
card('golden_egg','Trứng Vàng',1,'Nhận 1 năng lượng lượt sau. Tiêu hao.',[effect('next_energy',1)],'rare',exhaust=True)
card('whirlwind','Gió Xoáy',2,'Gây 8 sát thương lên mọi địch.',[damage(8,'all')],'rare',ranged=True)
card('guard_stance','Thủ Thế',2,'Nhận 12 giáp và rút 1 lá.',[effect('block',12),effect('draw',1)],'rare')
card('rage_fire','Nộ Hỏa',1,'Nhận 25 Nộ và 1 Suy Yếu.',[effect('rage',25),status('weak',1,'self')],'rare')
card('mark_prey','Đánh Dấu Mồi',1,'Gây 2 Dễ Vỡ cho mục tiêu.',[status('vulnerable',2)],'rare')
card('returning_spear','Lao Hồi Mã',1,'Gây 7 sát thương. Trở về tay nếu kết liễu.',[damage(7)],'rare',ranged=True,returnOnKill=True)
card('holy_spear','Thánh Giáo',3,'Mỗi lượt, lá tấn công đầu tiên gây gấp đôi sát thương.',[effect('double_attack',2)],'legendary',power=True,requiresSkill='chick_spear_5',**{'class':'chick'})
card('fortress','Pháo Đài Gà',2,'Cuối mỗi lượt nhận 4 giáp.',[effect('end_block',4)],'legendary',power=True,requiresSkill='chick_shield_5',**{'class':'chick'})
card('meteor_rain','Mưa Sao Băng',3,'Gây 4 sát thương ×6 lên địch ngẫu nhiên.',[damage(4,'random',times=6)],'legendary',ranged=True)
card('phoenix','Tái Sinh Phượng',2,'Lần chết đầu trong run, hồi sinh với 30% HP. Tiêu hao.',[effect('revive',1)],'legendary',exhaust=True)
card('resonance','Cộng Hưởng Sao',1,'Tuyệt kỹ cần ít hơn 20 Nộ trong trận này.',[effect('ult_discount',20)],'legendary',power=True,requiresSkill='chick_star_5',**{'class':'chick'})
card('mud','Bùn Lầy',0,'Không chơi được. Chiếm một chỗ trên tay.',[],'curse',unplayable=True)
card('fatigue','Mệt Mỏi',1,'Không có hiệu ứng. Tiêu hao.',[],'curse',exhaust=True)
card('spellbolt','Tia Phép',1,'Gây 7 sát thương phép.',[damage(7)],**{'class':'mage','ranged':True})
card('venom_orb','Cầu Độc',1,'Gây 4 Độc cho mục tiêu.',[status('poison',4)],**{'class':'mage','ranged':True})
card('mana_barrier','Màn Mana',0,'Nhận 4 giáp.',[effect('block',4)],**{'class':'mage'})
card('mana_focus','Tụ Mana',0,'Nhận 1 năng lượng. Tiêu hao.',[effect('energy',1)],**{'class':'mage','exhaust':True})
card('arcane_storm','Bão Phép',2,'Gây 5 sát thương và 2 Độc lên mọi địch.',[damage(5,'all'),effect('apply',2,'all',status='poison')],'rare',**{'class':'mage','ranged':True})
card('star_orbit','Quỹ Đạo Sao',1,'Rút 2 lá và nhận 10 Nộ.',[effect('draw',2),effect('rage',10)],'rare',**{'class':'mage'})
card('dagger','Dao Lông',0,'Gây 3 sát thương.',[damage(3)],**{'class':'ninja'})
card('smoke_step','Bước Khói',1,'Nhận 7 giáp và rút 1 lá.',[effect('block',7),effect('draw',1)],**{'class':'ninja'})
card('shadow_dance','Vũ Ảnh',0,'Rút 2 lá. Tiêu hao.',[effect('draw',2)],**{'class':'ninja','exhaust':True})
card('blade_rain','Mưa Dao',1,'Gây 3 sát thương ×3 lên địch ngẫu nhiên.',[damage(3,'random',times=3)],'rare',**{'class':'ninja','ranged':True})
card('assassinate','Ám Sát',2,'Gây 14 sát thương xuyên giáp.',[damage(14,pierce=True)],'rare',**{'class':'ninja'})
card('phantom_guard','Ảnh Khiên',1,'Nhận 5 giáp và giữ giáp một lượt.',[effect('block',5),effect('retain',1)],'rare',**{'class':'ninja'})
card('comet_lance','Lao Sao Chổi',2,'Gây 18 sát thương. Tiêu hao.',[damage(18)],'rare',ranged=True,exhaust=True)
card('lotus_guard','Khiên Liên Hoa',1,'Nhận 8 giáp và hồi 3 HP. Tiêu hao.',[effect('block',8),effect('heal',3)],'rare',exhaust=True)
assert len(cards)==50 and len({c['id'] for c in cards})==50
write('cards.json',cards)

classes=[
 dict(id='chick',name='Gà Chiến Binh',sprite='chick',hp=50,energy=3,draw=5,ultimate='spear',ultimateDamage=40,ultimateHits=1,description='Giáo và Khiên. Sao Rơi gây 40 sát thương.',deck=['punch']*5+['shield']*3+['heavy','cookie']),
 dict(id='mage',name='Gà Pháp Sư',sprite='mage',hp=40,energy=3,draw=6,mana=True,ultimate='storm',ultimateDamage=12,ultimateHits=4,description='Mana giữ lại tối đa 3. Bão Lông đánh 12 ×4 ngẫu nhiên.',deck=['spellbolt']*4+['mana_barrier']*3+['venom_orb','star_orbit','cookie']),
 dict(id='ninja',name='Gà Ninja',sprite='ninja',hp=45,energy=3,draw=5,ultimate='shadow',ultimateDamage=1,ultimateHits=3,description='Lá miễn phí và combo. Ảnh Phân lặp ba lá đã chơi gần nhất.',deck=['dagger']*4+['smoke_step']*3+['blade_rain','shadow_dance','cookie'])]
write('classes.json',classes)
skills=[]
branches={
 'chick':[('spear','Giáo',[{'spear_bonus':1},{'max_hp':2},{'third_attack':5},{'damage':1},{}],'holy_spear'),('shield','Khiên',[{'start_block':5},{'max_hp':3},{'retain_block':3},{'rest_bonus':3},{}],'fortress'),('star','Sao',[{'start_rage':10},{'max_hp':2},{'ultimate_bonus':10},{'start_rage':10},{'ultimate_vulnerable':2}],'resonance')],
 'mage':[('arcane','Phép',[{'damage':1},{'max_hp':2},{'draw':1},{'ultimate_bonus':2},{'ultimate_bonus':2}],''),('poison','Độc',[{'poison_bonus':1},{'max_hp':2},{'start_block':4},{'poison_bonus':1},{'first_attack':4}],''),('mana','Mana',[{'max_mana':1},{'rest_bonus':3},{'start_rage':15},{'max_hp':3},{'energy':1}],'')],
 'ninja':[('blade','Dao',[{'damage':1},{'max_hp':2},{'third_attack':5},{'first_attack':3},{'third_attack':3}],''),('mist','Khói',[{'start_block':5},{'max_hp':3},{'retain_block':3},{'rest_bonus':3},{'block':2}],''),('shadow','Ảnh',[{'start_rage':10},{'max_hp':2},{'ultimate_discount':10},{'draw':1},{'ultimate_discount':10}],'')]}
bonus_names={'spear_bonus':'sát thương Chọc Giáo','max_hp':'HP tối đa','third_attack':'sát thương đòn tấn công thứ ba/lượt','damage':'sát thương mỗi hit','start_block':'giáp đầu trận','retain_block':'giáp giữ qua lượt','rest_bonus':'HP hồi thêm khi nghỉ','start_rage':'Nộ đầu trận','ultimate_bonus':'sát thương mỗi hit tuyệt kỹ','ultimate_vulnerable':'Dễ Vỡ sau tuyệt kỹ','draw':'lá rút mỗi lượt','poison_bonus':'Độc mỗi lần áp từ bài','first_attack':'sát thương lá tấn công đầu trận','max_mana':'sức chứa Mana','energy':'năng lượng mỗi lượt','block':'giáp mỗi lần chơi lá thủ','ultimate_discount':'Nộ giảm cho tuyệt kỹ'}
for class_id,rows in branches.items():
    for branch,label,levels,unlock in rows:
        for tier,bonuses in enumerate(levels,1):
            unlock_card=unlock if tier==5 else ''
            desc=', '.join(f'+{n} {bonus_names[k]}' for k,n in bonuses.items()) or 'Mở lá năng lực trong phần thưởng.'
            if unlock_card:desc+=' Mở '+next(c['name'] for c in cards if c['id']==unlock_card)+'.'
            skills.append(dict(id=f'{class_id}_{branch}_{tier}',**{'class':class_id},branch=branch,tier=tier,cost=4+tier*3,name=f'{label} {tier}',description=desc,bonuses=bonuses,unlockCard=unlock_card))
write('skills.json',skills)
base_enemies=[e for e in json.loads((DATA/'enemies.json').read_text(encoding='utf-8')) if e['role'] in ('combat','elite') and e['id']!='circus_dummy']
for enemy in base_enemies:enemy['sprite']=enemy['id']
def attack(lo,hi,**extra):return dict(kind='attack',minDamage=lo,maxDamage=hi,weight=3,**extra)
def phase(name,hp,threshold,patterns,**extra):return dict(name=name,maxHp=hp,threshold=threshold,patterns=patterns,openingAttack=True,maxConsecutiveGuards=1,**extra)
bosses=[
 dict(id='frog',name='Ếch Vương',role='boss',sprite='frog_king',mechanic='frog',maxHp=150,openingAttack=True,maxConsecutiveGuards=1,patterns=[attack(8,13),dict(kind='defend',block=6,weight=1)],phases=[phase('Ếch Vương Cuồng Nộ',150,50,[attack(10,14),dict(kind='defend',block=8,weight=1)],lockCard=True,summon='tadpole',summonCount=2)]),
 dict(id='snake',name='Rắn Bùn Mẹ',role='boss',sprite='snake',mechanic='snake',maxHp=180,openingAttack=True,maxConsecutiveGuards=1,patterns=[attack(9,13,status='poison',amount=2),dict(kind='debuff',status='poison',amount=3,weight=1)],phases=[phase('Rắn Bùn Lột Da',180,65,[attack(11,15),dict(kind='buff',status='strength',amount=2,block=5,weight=1)],cleanse=True,strength=3),phase('Siết Nghẹt',180,25,[dict(kind='charge',minDamage=25,maxDamage=30,chargeTurns=2,weight=2),attack(12,16)],constrict=3)]),
 dict(id='hawk',name='Diều Hâu Bão',role='boss',sprite='hawk',mechanic='hawk',airborne=True,maxHp=160,openingAttack=True,maxConsecutiveGuards=1,patterns=[dict(kind='multi',minDamage=5,maxDamage=7,hits=2,weight=3),attack(12,16),dict(kind='buff',status='strength',amount=1,weight=1)]),
 dict(id='fox',name='Cáo Thầy Phù',role='boss',sprite='fox',mechanic='fox',maxHp=220,openingAttack=True,maxConsecutiveGuards=1,patterns=[attack(10,14),dict(kind='curse',weight=2)],phases=[phase('Cáo Ảo Ảnh',220,50,[attack(12,16),dict(kind='curse',weight=1)],summon='fox_clone',summonCount=2)]),
 dict(id='undergod',name='Undergod',role='boss',sprite='undergod',mechanic='undergod',maxHp=300,openingAttack=True,maxConsecutiveGuards=1,patterns=[dict(kind='mimic',minDamage=10,maxDamage=14,weight=4),dict(kind='defend',block=10,weight=1)],phases=[phase('Undergod Hút Nộ',300,66,[dict(kind='drain',minDamage=10,maxDamage=14,amount=20,weight=3),attack(14,18)]),phase('Tận Thế',300,33,[attack(16,20),dict(kind='multi',minDamage=6,maxDamage=8,hits=2,weight=2)],doom=5)])]
bosses.append(dict(id='tiger',name='Hổ Vương Xích Diệm',role='boss',sprite='tiger',mechanic='tiger',maxHp=200,openingAttack=True,maxConsecutiveGuards=1,patterns=[attack(10,13),dict(kind='multi',minDamage=4,maxDamage=6,hits=2,weight=2),dict(kind='buff',status='strength',amount=2,block=6,weight=1)],phases=[phase('Hổ Vương Bùng Lửa',200,50,[dict(kind='charge',minDamage=25,maxDamage=28,chargeTurns=2,weight=1)],strength=2)]))
summons=[dict(id='tadpole',name='Nòng Nọc',role='summon',sprite='tadpole',maxHp=12,openingAttack=True,maxConsecutiveGuards=0,patterns=[attack(3,4)]),dict(id='fox_clone',name='Phân Thân Cáo',role='summon',sprite='fox',maxHp=1,openingAttack=True,maxConsecutiveGuards=0,patterns=[attack(1,2)]),dict(id='circus_dummy',name='Võ Sĩ Xiếc',role='combat',sprite='elite_frog',maxHp=30,openingAttack=True,maxConsecutiveGuards=0,patterns=[attack(4,6)])]
write('enemies.json',base_enemies+bosses+summons)
regular=[e['id'] for e in base_enemies if e['role']=='combat'];elites=[e['id'] for e in base_enemies if e['role']=='elite']
acts=[dict(id='meadow',name='Đồng Cỏ Bình Minh',biome='meadow',bosses=['frog'],combats=regular,elites=elites,floors=12,columns=3),dict(id='marsh',name='Đầm Lầy Giông Bão',biome='marsh',bosses=['snake','hawk'],combats=regular,elites=elites,floors=12,columns=3),dict(id='temple',name='Đền Sao Tàn',biome='temple',bosses=['fox'],finalBoss='undergod',combats=regular,elites=elites,floors=12,columns=3)]
acts[1]['bosses'].append('tiger')
write('campaign.json',acts)
relics=[
 dict(id='down_feather',name='Lông Đuôi Phượng',rarity='common',price=150,description='Đầu trận nhận 8 giáp.',onCombatStart=[effect('block',8)]),
 dict(id='coin_pouch',name='Hạt Ngô Vàng',rarity='common',price=150,goldBonus=15,description='Mỗi trận thắng nhận thêm 15 vàng.'),
 dict(id='steel_beak',name='Mỏ Sắt',rarity='common',price=150,firstAttack=4,description='Lá tấn công đầu mỗi trận gây thêm 4 sát thương.'),
 dict(id='water_jar',name='Tổ Ấm',rarity='common',price=150,restPercent=10,description='Chỗ nghỉ hồi thêm 10% HP tối đa.'),
 dict(id='lucky_spur',name='Chuông Bình Minh',rarity='rare',price=200,description='Lượt đầu trận rút thêm 2 lá.',onCombatStart=[effect('draw',2)]),
 dict(id='shell_shield',name='Khiên Vỏ Ốc',rarity='rare',price=200,retainBlock=5,description='Giữ tối đa 5 giáp sang lượt sau.'),
 dict(id='rage_pearl',name='Ngọc Nộ',rarity='rare',price=200,startRage=30,description='Bắt đầu trận với ít nhất 30 Nộ.'),
 dict(id='frog_crown',name='Vương Miện Ếch',rarity='boss',price=250,energy=1,draw=-1,description='Mỗi lượt +1 năng lượng, rút ít hơn 1 lá.'),
 dict(id='hawk_eye',name='Mắt Diều Hâu',rarity='boss',price=250,preview=True,description='Xem dự kiến hai lượt tới nếu không chơi thêm bài.'),
 dict(id='meteor_spear',name='Giáo Sao Băng',rarity='boss',price=250,comet=True,description='Tuyệt kỹ sát thương đánh hai lần, mỗi lần 60%.'),
 dict(id='warm_egg',name='Trứng Ấm',rarity='rare',price=220,description='Đầu mỗi lượt nhận thêm 1 năng lượng.',onTurnStart=[effect('energy',1)]),
 dict(id='thorn_shield',name='Khiên Gai',rarity='common',price=150,description='Đầu mỗi lượt nhận 3 giáp.',onTurnStart=[effect('block',3)]),
 dict(id='venom_vial',name='Lọ Nọc',rarity='rare',price=190,description='Đầu trận gây 4 Độc.',onCombatStart=[status('poison',4)]),
 dict(id='brave_heart',name='Tim Dũng',rarity='common',price=160,maxHp=8,description='Nhận thêm 8 HP tối đa.'),
 dict(id='bargain',name='Bùa Mặc Cả',rarity='common',price=150,shopDiscount=20,description='Giảm 20% giá mua ở cửa hàng.'),
 dict(id='bandolier',name='Dây Thuốc',rarity='common',price=150,potionSlot=1,description='Thêm một ô thuốc.'),
 dict(id='war_drum',name='Trống Trận',rarity='rare',price=180,description='Đầu trận nhận 1 Sức Mạnh.',onCombatStart=[status('strength',1,'self')])]
write('relics.json',relics)
def choice(label,outcome,**extra):return dict(label=label,outcome=outcome,**extra)
events=[
 dict(id='well',title='Giếng Ước',text='Một đồng xu đổi lấy cơ hội. Hay uống nước và bước tiếp?',choices=[choice('Ném 30 vàng','50% nhận một di vật thường.',gold=-30,action='wish',chance=.5),choice('Uống nước','Hồi 15 HP.',heal=15),choice('Bỏ qua','Không thay đổi.')]),
 dict(id='forge',title='Lò Rèn Cũ',text='Ngọn lửa còn đủ cho một lần rèn.',choices=[choice('Nâng cấp một lá','Chọn lá muốn nâng cấp.',action='select_upgrade'),choice('Bỏ một lá','Chọn lá muốn gỡ.',action='select_remove'),choice('Rèn giáo','Mọi Chọc Giáo +1 sát thương. Mất 8 HP.',damage=8,action='forge_spear',amount=1)]),
 dict(id='egg',title='Ổ Trứng Lạ',text='Quả trứng tỏa ánh vàng, kèm một cái bóng.',choices=[choice('Ấp trứng','Nhận một lá Huyền thoại và một Bùn Lầy.',action='egg'),choice('Ăn trứng','Tăng 6 HP tối đa.',maxHp=6)]),
 dict(id='merchant',title='Thương Nhân Rong',text='Thương nhân muốn đổi một món đồ của bạn lấy hai món khác.',choices=[choice('Đổi một di vật','Chọn di vật để đổi lấy hai di vật thường.',action='select_trade'),choice('Bỏ đi','Không thay đổi.')]),
 dict(id='lost_frog',title='Ếch Con Lạc',text='Một chú ếch con đang tìm đường về nhà.',choices=[choice('Đưa về','Có đồng hành hồi 2 HP sau mỗi trận.',action='companion',amount=2),choice('Lấy đồ','Nhận 60 vàng và một Bùn Lầy.',gold=60,card='mud')]),
 dict(id='star_temple',title='Đền Sao',text='Những vì sao đáp lời một sự hiến dâng.',choices=[choice('Hiến 10 HP tối đa','Tuyệt kỹ +15 sát thương mỗi hit.',action='star_sacrifice',amount=15),choice('Cầu nguyện','Đầu mỗi trận nhận thêm 20 Nộ.',action='start_rage',amount=20)]),
 dict(id='bridge',title='Cầu Gãy',text='Bên dưới cây cầu là một con đường hiểm trở.',choices=[choice('Liều qua','50% đánh tinh anh, thưởng gấp đôi; thất bại đánh trận thường.',action='bridge',chance=.5),choice('Đi vòng','Đánh thêm một trận thường.',action='detour')]),
 dict(id='circus',title='Gánh Xiếc Gà',text='Hạ Võ Sĩ Xiếc trong ba lượt để thắng giải.',choices=[choice('Thi đấu','Thử thách chiến đấu ba lượt. Thắng nhận 100 vàng.',action='trial',amount=3),choice('Xem biểu diễn','Nhận một lá ngẫu nhiên.',action='random_card')]),
 dict(id='mist',title='Sương Mù',text='Sương che lối đi, nhưng để lộ một chiếc lọ.',choices=[choice('Đi vào sương','Ẩn loại nút hai tầng tiếp theo, nhận một thuốc.',action='fog',amount=2),choice('Chờ sương tan','Không thay đổi.')]),
 dict(id='pact',title='Bóng Undergod',text='Một bàn tay đen đưa ra bản khế ước.',choices=[choice('Ký khế ước','+1 năng lượng mỗi lượt; Undergod cuối +25% HP.',action='pact'),choice('Từ chối','Không thay đổi.')]),
 dict(id='library',title='Thư Viện Lông Vũ',text='Những trang sách giữ lại kinh nghiệm của người đi trước.',choices=[choice('Chọn sách','Chọn một trong ba lá Hiếm.',action='rare_offer'),choice('Học nâng cấp','Chọn một lá để nâng cấp.',action='select_upgrade')]),
 dict(id='ambush',title='Ổ Phục Kích',text='Hai bóng địch hiện ra trên con đường hẹp.',choices=[choice('Đột phá','Đánh hai trận liên tiếp, không hồi giữa trận. Nhận di vật.',action='ambush'),choice('Rút lui','Mất 8 HP để thoát.',damage=8)])]
write('events.json',events)
challenges=[('Địch +10% HP.',{'enemy_hp':10}),('Địch gây thêm 1 sát thương mỗi hit.',{'enemy_damage':1}),('Nghỉ hồi ít hơn 5% HP tối đa.',{'rest_penalty':5}),('Tinh anh +15% HP.',{'elite_hp':15}),('Bắt đầu ít hơn 25 vàng.',{'start_gold':-25}),('Địch gây thêm 1 sát thương mỗi hit.',{'enemy_damage':1}),('Mất 5 HP tối đa.',{'player_hp':-5}),('Ít hơn một ô thuốc.',{'potion_slots':-1}),('Boss gây thêm 2 sát thương mỗi hit.',{'boss_damage':2}),('Boss +25% HP.',{'boss_hp':25})]
write('challenges.json',[dict(level=i,description=text,penalties=penalty) for i,(text,penalty) in enumerate(challenges,1)])
achievements=[('first_boss','Ngôi Sao Đầu Tiên','bosses',1,5),('campaign','Bình Minh Trở Lại','wins',1,15),('warrior_win','Chiến Binh Bất Khuất','win_chick',1,8),('mage_win','Bão Lông','win_mage',1,8),('ninja_win','Ảnh Qua Đêm','win_ninja',1,8),('poison','Màu Xanh Nguy Hiểm','poison',100,5),('meteor','Sao Không Tắt','ultimates',10,5),('revival','Từ Tro Tàn','revives',1,8),('challenge1','Bước Vào Thử Thách','challenge',1,8),('challenge5','Vượt Giông','challenge',5,15),('challenge10','Chạm Đỉnh','challenge',10,25),('all_bosses','Biết Mọi Kẻ Thù','unique_bosses',5,15),('damage','Mũi Giáo Sắc','damage',1000,5),('collector','Kho Báu','unique_relics',10,8),('skill','Học Một Điều Mới','skills',1,3)]
achievement_texts={'first_boss': 'Hạ một trùm.', 'campaign': 'Hoàn tất cả ba hồi.', 'warrior_win': 'Thắng chiến dịch với Gà Chiến Binh.', 'mage_win': 'Thắng chiến dịch với Gà Pháp Sư.', 'ninja_win': 'Thắng chiến dịch với Gà Ninja.', 'poison': 'Tích lũy 100 Độc gây lên địch.', 'meteor': 'Thi triển tổng cộng 10 tuyệt kỹ.', 'revival': 'Hồi sinh bằng Tái Sinh Phượng.', 'challenge1': 'Thắng Thử Thách 1.', 'challenge5': 'Thắng Thử Thách 5.', 'challenge10': 'Thắng Thử Thách 10.', 'all_bosses': 'Hạ đủ năm loại trùm qua các hành trình.', 'damage': 'Gây tổng cộng 1.000 sát thương.', 'collector': 'Thu thập 10 di vật khác nhau.', 'skill': 'Mở một nút kỹ năng.'}
achievement_texts['all_bosses']='Hạ đủ sáu loại trùm qua các hành trình.'
write('achievements.json',[dict(id=key,name=name,description=achievement_texts[key],metric=metric,threshold=len(bosses) if key=='all_bosses' else threshold,feathers=reward) for key,name,metric,threshold,reward in achievements])
print(f'Content: {len(cards)} base cards, {len(classes)} classes, {len(skills)} skills, {len(acts)} acts, {len(events)} events, {len(relics)} relics.')
