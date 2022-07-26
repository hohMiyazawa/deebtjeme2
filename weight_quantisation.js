let quant_cost = function(num,weight,total){
	let cost1 = num * Math.log2(weight/(total + weight - num))
		+ (total - num) * Math.log2((total - num)/(total + weight - num));
	let cost2 = num * Math.log2(num/total)
		+ (total - num) * Math.log2((total - num)/total);
	console.log(cost1,cost2,cost1 - cost2)
}

/*
a,b,c


a * log(b/(c + b - a))
+ (c - a) * log((c - a)/(c + b - a))

- a * log(a/c)
- (c - a) * log((c - a)/c)

---

a * log(b) - a * log(c + b - a)
+ (c - a) * log(c - a) - (c - a) * log(c + b - a)
- a * log(a) + a * log(c)
- (c - a) * log(c - a) + (c - a) * log(c)

a * log(b) + a * log(c)
 - c * log(c + b - a)
- a * log(a)
 + (c - a) * log(c)

diff relevant:

a * log(b)
 - c * log(c + b - a)

as cost:


 c * log(c + b - a)
 - a * log(b)

~
(b-a) * log2(e)
 - a * log(b)

as diff

diff * log2(e)
 - 2 * (log(a + diff) - log(a))


let cost = function(a,unoy){
	let con = unoy * Math.log2(Math.E);
	console.log(
		con + a*(Math.log2(a) - Math.log2(a + unoy)),
		con - a*(Math.log2(a) - Math.log2(a - unoy)),
	)
}

*/
